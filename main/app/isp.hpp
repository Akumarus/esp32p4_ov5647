#pragma once

#include "driver/isp.h"
#include <cstdint>

class Isp {
public:
    explicit Isp(esp_isp_processor_cfg_t &cfg);
    ~ Isp();

    isp_proc_handle_t getHandle() const;
    static esp_isp_processor_cfg_t getDefaultConfig(uint32_t width, uint32_t height);
private:
    isp_proc_handle_t handle = nullptr;
};