#pragma once

#include "driver/i2c_master.h"

class I2c {
public:
    I2c();
    i2c_master_bus_handle_t getHandle();

private:
    i2c_master_bus_handle_t handle = nullptr;
};