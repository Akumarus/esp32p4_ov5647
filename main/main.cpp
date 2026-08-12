#include <stdio.h>

#include "driver/i2c_master.h" 
#include "esp_sccb_intf.h" 
#include "esp_sccb_i2c.h"

#include "esp_cam_sensor.h"
#include "esp_cam_sensor_types.h"
#include "ov5647.h" 

#include "esp_ldo_regulator.h"

extern "C" void app_main(void)
{
    // esp_ldo_channel_config_t ldo_cfg = {
    //     chn_id
    // }

    i2c_master_bus_handle_t bus_handle;
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = I2C_NUM_0;
    bus_cfg.sda_io_num = GPIO_NUM_7;
    bus_cfg.scl_io_num = GPIO_NUM_8;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = false;
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
    sensor_cfg.xclk_freq_hz = 240000000;
    sensor_cfg.sensor_port = ESP_CAM_SENSOR_MIPI_CSI;

    esp_cam_sensor_device_t *dev = ov5647_detect(&sensor_cfg);
    if (dev == nullptr) {
        ESP_LOGI("Main", "OV5647 не обнаружен");
    }
    ESP_LOGI("Main", "OV5647 обнаружен, name=%s", dev->name);
}
