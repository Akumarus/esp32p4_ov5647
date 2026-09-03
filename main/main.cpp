#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h" 
// #include "esp_sccb_intf.h" 
// #include "esp_sccb_i2c.h"

#include "esp_cam_sensor.h"
#include "esp_cam_sensor_types.h"
#include "ov5647.h" 

#include "esp_ldo_regulator.h"

#include "driver/sdmmc_host.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "esp_vfs_fat.h"
#include "esp_cache.h"

#include "sensor.hpp"
#include "bmp.hpp"
#include "isp.hpp"
#include "csi.hpp"

extern "C" void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_ldo_channel_handle_t ldo_handler;
    esp_ldo_channel_config_t ldo_cfg = {};
    ldo_cfg.chan_id = 3;
    ldo_cfg.voltage_mv = 2500;
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_cfg, &ldo_handler));

    i2c_master_bus_handle_t bus_handle;
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = I2C_NUM_0;
    bus_cfg.sda_io_num = GPIO_NUM_7;
    bus_cfg.scl_io_num = GPIO_NUM_8;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));
    
    /* Sensor initialization */
    Sensor ov5647(bus_handle);
    ov5647.setFormat(800, 800);
    ov5647.enable();
    /* ISP initialization */
    auto ispConfig = Isp::getDefaultConfig(800, 800);
    Isp isp(ispConfig);
    /* Csi initialization */
    auto csiConfig = Csi::getDefaultConfig(800, 800);
    Csi csi(csiConfig);
    csi.enable();
    csi.start();
    csi.receive(ESP_CAM_CTLR_MAX_DELAY);

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
    
    while (csi.counter == 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    esp_cache_msync(csi.trans.buffer, csi.trans.buflen, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    uint32_t bmpSize = Bmp::encodedSize(800, 800);
    uint8_t *bmpData = (uint8_t *)heap_caps_aligned_alloc(8, bmpSize, MALLOC_CAP_SPIRAM);
    Bmp::save(bmpData, bmpSize, (uint16_t *)csi.trans.buffer, 800, 800);
    FILE *f = fopen("/sdcard/frame.bmp", "wb");
    if (f != NULL) {
        fwrite(bmpData, 1, bmpSize, f);
        fclose(f);
        ESP_LOGI("Main", "BMP сохранен");
    } else {
        ESP_LOGE("Main", "Ошибка открытия файла");
    }

    while (true)
    {
        ESP_LOGI("Main", "frame count = %lu", csi.counter);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
