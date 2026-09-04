#include "ldo.hpp"
#include "esp_ldo_regulator.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"

Ldo::Ldo(int id)
{
    sd_pwr_ctrl_handle_t h = nullptr;
    sd_pwr_ctrl_ldo_config_t config = { .ldo_chan_id = id };
    ESP_ERROR_CHECK(sd_pwr_ctrl_new_on_chip_ldo(&config, &h));
    deleter = [h]() { sd_pwr_ctrl_del_on_chip_ldo(h); };
    handle = h;
}

Ldo::Ldo(int id, int voltage)
{
    esp_ldo_channel_handle_t h = nullptr;
    esp_ldo_channel_config_t config = {};
    config.chan_id = id;
    config.voltage_mv = voltage;
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&config, &h));
    handle = h;
    
}

Ldo::~Ldo()
{
    if (deleter)
        deleter();
}