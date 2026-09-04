#include "sensor.hpp"
#include "ov5647.h"
#include "esp_log.h"

Sensor::Sensor() : ldo(3, 2500), i2c{}
{
    sccb_i2c_config_t sccbConfig = {};
    sccbConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    sccbConfig.device_address = OV5647_ADDR;
    sccbConfig.scl_speed_hz = SENSOR_SCCB_SPEED;
    ESP_ERROR_CHECK(sccb_new_i2c_io(i2c.getHandle(), &sccbConfig, &sccbHandle));

    esp_cam_sensor_config_t sensorConfig = {};
    sensorConfig.sccb_handle = sccbHandle;
    sensorConfig.reset_pin = GPIO_NUM_NC;
    sensorConfig.pwdn_pin = GPIO_NUM_NC;
    sensorConfig.xclk_pin = GPIO_NUM_NC;
    sensorConfig.xclk_freq_hz = SENSOR_XLCLK_FREQ;
    sensorConfig.sensor_port = ESP_CAM_SENSOR_MIPI_CSI;
    dev = ov5647_detect(&sensorConfig);
    
    if (dev == nullptr) 
        ESP_LOGI("Main", "Сенсор не обнаружен");
    ESP_LOGI("Main", "Сенсор обнаружен, name=%s", getName());
}

Sensor::~Sensor()
{
    if (dev) {
        esp_cam_sensor_del_dev(dev);
    }
}
const char* Sensor::getName()
{
    return dev->name;
}

esp_cam_sensor_format_t Sensor::getFormat()
{
    esp_cam_sensor_format_t format = {};
    ESP_ERROR_CHECK(dev->ops->get_format(dev, &format));
    return format;
}

bool Sensor::setFormat(uint32_t width, uint32_t height)
{
    ESP_ERROR_CHECK(dev->ops->query_support_formats(dev, &formats));
    for (uint16_t i = 0; i < formats.count; i++) {
        auto &format = formats.format_array[i];
        ESP_LOGI("Sensor", "Format %u: Width = %u, Height = %u", i, format.width, format.height);
        if (format.width == width && format.height == height) {
            ESP_ERROR_CHECK(dev->ops->set_format(dev, &format));
            return true;
        }
    }
    return false;
}

void Sensor::enable()
{
    int enable = 1;
    dev->ops->priv_ioctl(dev, ESP_CAM_SENSOR_IOC_S_STREAM, &enable);
}