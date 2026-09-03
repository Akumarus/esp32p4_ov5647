#pragma once

#include "driver/i2c_master.h"
#include "esp_sccb_i2c.h"
#include "esp_cam_sensor.h"
#include <cstdint>

class Sensor {
public:
    Sensor(i2c_master_bus_handle_t busHandle);
    ~Sensor();
    
    esp_cam_sensor_format_t getFormat();
    bool setFormat(uint32_t width, uint32_t height);
    const char* getName();
    bool isDetected();
    void enable();

private:
    esp_sccb_io_handle_t sccbHandle = {};
    esp_cam_sensor_device_t *dev = nullptr;
    esp_cam_sensor_format_array_t formats = {};

    static constexpr uint16_t OV5647_ADDR = 0x36;
    static constexpr uint32_t SENSOR_SCCB_SPEED = 100000;
    static constexpr uint32_t SENSOR_XLCLK_FREQ = 24000000;
};