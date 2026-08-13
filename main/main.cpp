#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h" 
#include "esp_sccb_intf.h" 
#include "esp_sccb_i2c.h"

#include "esp_cam_sensor.h"
#include "esp_cam_sensor_types.h"
#include "ov5647.h" 

#include "esp_ldo_regulator.h"
#include "esp_cam_ctlr.h"
#include "esp_cam_ctlr_types.h"
#include "esp_cam_ctlr_csi.h"

static QueueHandle_t s_frame_evt_queue;

static bool IRAM_ATTR on_cam_trans_finished(esp_cam_ctlr_handle_t handle,
                                             esp_cam_ctlr_trans_t *trans,
                                             void *data)
{
    ESP_LOGI("MAIN", "CALLBACK");
    BaseType_t hp_task_woken = pdFALSE;
    xQueueSendFromISR(s_frame_evt_queue, &trans->buffer, &hp_task_woken);
    return hp_task_woken == pdTRUE;
}

static bool IRAM_ATTR on_cam_trans_started(esp_cam_ctlr_handle_t handle,
                                            esp_cam_ctlr_trans_t *trans,
                                            void *data)
{
    return pdTRUE;
}


extern "C" void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    s_frame_evt_queue = xQueueCreate(2, sizeof(void *));

    i2c_master_bus_handle_t bus_handle;
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = I2C_NUM_0;
    bus_cfg.sda_io_num = GPIO_NUM_7;
    bus_cfg.scl_io_num = GPIO_NUM_8;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));
    
    esp_sccb_io_handle_t sccb_handle;
    sccb_i2c_config_t sccb_cfg = {};
    sccb_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    sccb_cfg.device_address = 0x36;
    sccb_cfg.scl_speed_hz = 100000;
    ESP_ERROR_CHECK(sccb_new_i2c_io(bus_handle, &sccb_cfg, &sccb_handle));

    esp_cam_sensor_config_t sensor_cfg = {};
    sensor_cfg.sccb_handle = sccb_handle;
    sensor_cfg.reset_pin = GPIO_NUM_NC;
    sensor_cfg.pwdn_pin = GPIO_NUM_NC;
    sensor_cfg.xclk_pin = GPIO_NUM_NC;
    sensor_cfg.xclk_freq_hz = 24000000;
    sensor_cfg.sensor_port = ESP_CAM_SENSOR_MIPI_CSI;

    esp_cam_sensor_device_t *dev = ov5647_detect(&sensor_cfg);
    if (dev == NULL) {
        ESP_LOGI("Main", "OV5647 не обнаружен");
    }
    ESP_LOGI("Main", "OV5647 обнаружен, name=%s", dev->name);

    esp_cam_sensor_format_array_t formats = {};
    ESP_ERROR_CHECK(dev->ops->query_support_formats(dev, &formats));

    const esp_cam_sensor_format_t *chosen = nullptr;
    for (int i = 0; i < formats.count; i++) {
        const auto &f = formats.format_array[i];
        ESP_LOGI("Main", "format[%d]: %s %lu x %lu", i, f.name, (uint32_t)f.width, (uint32_t)f.height);
        if (f.width == 800 && f.height == 800 && f.format == ESP_CAM_SENSOR_PIXFORMAT_RAW8) {
            chosen = &f;
            break;
        }
    }
    if (!chosen) { ESP_LOGE("Main", "нет подходящего формата"); return; }

    esp_cam_sensor_format_t format;
    ESP_ERROR_CHECK(dev->ops->set_format(dev, chosen));
    ESP_ERROR_CHECK(dev->ops->get_format(dev, &format));


    esp_ldo_channel_handle_t ldo_handler;
    esp_ldo_channel_config_t ldo_cfg = {};
    ldo_cfg.chan_id = 3;
    ldo_cfg.voltage_mv = 2500;
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_cfg, &ldo_handler));
    
    esp_cam_ctlr_handle_t csi_handler = nullptr;
    esp_cam_ctlr_csi_config_t csi_cfg = {};
    csi_cfg.ctlr_id = 0;
    csi_cfg.h_res = chosen->width;
    csi_cfg.v_res = chosen->height;
    csi_cfg.data_lane_num = 2;
    csi_cfg.input_data_color_type = CAM_CTLR_COLOR_RAW8;
    csi_cfg.output_data_color_type = CAM_CTLR_COLOR_RAW8;
    csi_cfg.lane_bit_rate_mbps = chosen->mipi_info.mipi_clk;
    csi_cfg.queue_items = 2;
    ESP_ERROR_CHECK(esp_cam_new_csi_ctlr(&csi_cfg, &csi_handler));

    esp_cam_ctlr_evt_cbs_t callbacks = {};
    callbacks.on_trans_finished = on_cam_trans_finished;
    callbacks.on_get_new_trans = on_cam_trans_started;

    ESP_ERROR_CHECK(esp_cam_ctlr_register_event_callbacks(csi_handler, &callbacks, NULL));
    
    
    uint32_t frame_size = chosen->width * chosen->height * 2;
    void *frame_data = heap_caps_aligned_alloc(64, frame_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (frame_data == nullptr) {
        ESP_LOGE("Main", "Не удалось выделить буффер размером %lu байт", frame_size);
        return;
    }

    esp_cam_ctlr_trans_t trans = {};
    trans.buffer = frame_data;
    trans.buflen = frame_size;

    ESP_ERROR_CHECK(esp_cam_ctlr_receive(csi_handler, &trans, ESP_CAM_CTLR_MAX_DELAY));

    ESP_ERROR_CHECK(esp_cam_ctlr_enable(csi_handler));
    ESP_ERROR_CHECK(esp_cam_ctlr_start(csi_handler));
    int enable = 1;
    dev->ops->priv_ioctl(dev, ESP_CAM_SENSOR_IOC_S_STREAM, &enable);


    while (true)
    {
        void *ready_buf = NULL; 
        if (xQueueReceive(s_frame_evt_queue, &ready_buf, pdMS_TO_TICKS(2000)) != pdTRUE) {
            ESP_LOGW("Main", "Таймаут ожидания кадра — проверь физическое подключение "
                          "CSI-шлейфа и питание сенсора");
            continue;
        }
    }
}
