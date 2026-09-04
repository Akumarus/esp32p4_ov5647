#include "i2c.hpp"

I2c::I2c()
{
    i2c_master_bus_config_t config = {};
    config.i2c_port = I2C_NUM_0;
    config.sda_io_num = GPIO_NUM_7;
    config.scl_io_num = GPIO_NUM_8;
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    config.flags.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_new_master_bus(&config, &handle));
}

i2c_master_bus_handle_t I2c::getHandle()
{
    return handle;
}