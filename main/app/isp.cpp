#include "isp.hpp"

Isp::Isp(esp_isp_processor_cfg_t &config)
{
    ESP_ERROR_CHECK(esp_isp_new_processor(&config, &handle));
    ESP_ERROR_CHECK(esp_isp_enable(handle));
}

Isp::~Isp()
{
    if (handle) {
        esp_isp_disable(handle);
        esp_isp_del_processor(handle);
    }
}

esp_isp_processor_cfg_t Isp::getDefaultConfig(uint32_t width, uint32_t height)
{
    esp_isp_processor_cfg_t cfg = {};
    cfg.clk_hz = 80000000;
    cfg.input_data_source = ISP_INPUT_DATA_SOURCE_CSI;
    cfg.input_data_color_type = ISP_COLOR_RAW8;
    cfg.output_data_color_type = ISP_COLOR_RGB565;
    cfg.has_line_start_packet = false;
    cfg.has_line_end_packet = false;
    cfg.h_res = width;
    cfg.v_res = height;
    return cfg;
}

isp_proc_handle_t Isp::getHandle() const
{
    return handle;
}