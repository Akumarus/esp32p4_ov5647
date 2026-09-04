#pragma once

#include "esp_sccb_i2c.h"
#include "esp_cam_sensor.h"
#include "ldo.hpp"
#include "i2c.hpp"
#include <cstdint>

class Sensor {
public:
    Sensor();
    ~Sensor();
    
    esp_cam_sensor_format_t getFormat();
    bool setFormat(uint32_t width, uint32_t height);
    const char* getName();
    bool isDetected();
    void enable();

private:
    Ldo ldo;
    I2c i2c;
    esp_sccb_io_handle_t sccbHandle = {};
    esp_cam_sensor_device_t *dev = nullptr;
    esp_cam_sensor_format_array_t formats = {};

    static constexpr uint16_t OV5647_ADDR = 0x36;
    static constexpr uint32_t SENSOR_SCCB_SPEED = 100000;
    static constexpr uint32_t SENSOR_XLCLK_FREQ = 24000000;
};