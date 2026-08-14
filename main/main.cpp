#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h" 
#include "esp_sccb_intf.h" 
#include "esp_sccb_i2c.h"

#include "esp_cam_sensor.h"
#include "esp_cam_sensor_types.h"
#include "ov5647.h" 
#include "driver/isp.h"

#include "esp_ldo_regulator.h"
#include "esp_cam_ctlr.h"
#include "esp_cam_ctlr_types.h"
#include "esp_cam_ctlr_csi.h"

#include "driver/sdmmc_host.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "esp_vfs_fat.h"

static QueueHandle_t s_frame_evt_queue;

volatile uint32_t counter = 0;

static bool IRAM_ATTR on_cam_trans_finished(esp_cam_ctlr_handle_t handle,
                                             esp_cam_ctlr_trans_t *trans,
                                             void *data)
{
    counter++;
    return false;
}

static bool IRAM_ATTR on_cam_trans_started(esp_cam_ctlr_handle_t handle,
                                            esp_cam_ctlr_trans_t *trans,
                                            void *data)
{
    esp_cam_ctlr_trans_t new_trans = *(esp_cam_ctlr_trans_t *)data;
    trans->buffer = new_trans.buffer;
    trans->buflen = new_trans.buflen;
    return false;
}


extern "C" void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_ldo_channel_handle_t ldo_handler;
    esp_ldo_channel_config_t ldo_cfg = {};
    ldo_cfg.chan_id = 3;
    ldo_cfg.voltage_mv = 2500;
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_cfg, &ldo_handler));

    uint32_t frame_size = 800 * 800 * 2;
    void *frame_data = heap_caps_aligned_alloc(64, frame_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (frame_data == nullptr) {
        ESP_LOGE("Main", "Не удалось выделить буффер размером %lu байт", frame_size);
        return;
    }

    esp_cam_ctlr_trans_t trans = {
        .buffer = frame_data,
        .buflen = frame_size,
    };

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
        return;
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

    int enable = 1;
    dev->ops->priv_ioctl(dev, ESP_CAM_SENSOR_IOC_S_STREAM, &enable);

    esp_cam_ctlr_handle_t csi_handler = nullptr;
    esp_cam_ctlr_csi_config_t csi_cfg = {};
    csi_cfg.ctlr_id = 0;
    csi_cfg.h_res = 800;
    csi_cfg.v_res = 800;
    csi_cfg.lane_bit_rate_mbps = 200;
    csi_cfg.input_data_color_type = CAM_CTLR_COLOR_RAW8;
    csi_cfg.output_data_color_type = CAM_CTLR_COLOR_RGB565;
    csi_cfg.data_lane_num = 2;
    csi_cfg.byte_swap_en = false;
    csi_cfg.queue_items = 1;
    ESP_ERROR_CHECK(esp_cam_new_csi_ctlr(&csi_cfg, &csi_handler));

    esp_cam_ctlr_evt_cbs_t callbacks = {};
    callbacks.on_trans_finished = on_cam_trans_finished;
    callbacks.on_get_new_trans = on_cam_trans_started;
    ESP_ERROR_CHECK(esp_cam_ctlr_register_event_callbacks(csi_handler, &callbacks, &trans));

    ESP_ERROR_CHECK(esp_cam_ctlr_enable(csi_handler));

    isp_proc_handle_t isp_handle = nullptr;
    esp_isp_processor_cfg_t isp_cfg = {};
    isp_cfg.clk_hz = 80000000;
    isp_cfg.input_data_source = ISP_INPUT_DATA_SOURCE_CSI;
    isp_cfg.input_data_color_type = ISP_COLOR_RAW8;
    isp_cfg.output_data_color_type = ISP_COLOR_RGB565;
    isp_cfg.has_line_start_packet = false;
    isp_cfg.has_line_end_packet = false;
    isp_cfg.h_res = 800;
    isp_cfg.v_res = 800;
    ESP_ERROR_CHECK(esp_isp_new_processor(&isp_cfg, &isp_handle));
    ESP_ERROR_CHECK(esp_isp_enable(isp_handle));
    ESP_ERROR_CHECK(esp_cam_ctlr_start(csi_handler));
    ESP_ERROR_CHECK(esp_cam_ctlr_receive(csi_handler, &trans, ESP_CAM_CTLR_MAX_DELAY));



    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = 4,
    };
    ESP_ERROR_CHECK(sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle));

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {};
    mount_cfg.format_if_mount_failed = false;
    mount_cfg.max_files = 5;
    mount_cfg.allocation_unit_size = 16 * 1024;

    sdmmc_card_t *card;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.pwr_ctrl_handle = pwr_ctrl_handle;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;
    slot_config.d0 = GPIO_NUM_39;
    slot_config.d1 = GPIO_NUM_40;
    slot_config.d2 = GPIO_NUM_41;
    slot_config.d3 = GPIO_NUM_42;
    slot_config.clk = GPIO_NUM_43;
    slot_config.cmd = GPIO_NUM_44;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_ERROR_CHECK(esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_cfg, &card));
    ESP_LOGI("Main", "SD-карта смонтирована");
    
    while (true)
    {
        ESP_LOGI("Main", "frame count = %lu", counter);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
