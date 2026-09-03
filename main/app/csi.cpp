#include "csi.hpp"

Csi::Csi(esp_cam_ctlr_csi_config_t &config)
{
    trans.buflen = config.h_res * config.v_res * 2;
    trans.buffer = heap_caps_aligned_alloc(64, trans.buflen, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (trans.buffer == nullptr) 
        abort();
    callbacks.on_get_new_trans = onTransStarted;
    callbacks.on_trans_finished = onTransFinished;
    ESP_ERROR_CHECK(esp_cam_new_csi_ctlr(&config, &handle));
    ESP_ERROR_CHECK(esp_cam_ctlr_register_event_callbacks(handle, &callbacks, this));
}

bool Csi::enable()
{
    return esp_cam_ctlr_enable(handle) == ESP_OK;
}

bool Csi::start()
{
    return esp_cam_ctlr_start(handle) == ESP_OK;
}
 
bool Csi::stop()
{
    return esp_cam_ctlr_stop(handle) == ESP_OK;
}
 
bool Csi::receive(uint32_t timeout)
{
    return esp_cam_ctlr_receive(handle, &trans, timeout) == ESP_OK;
}

esp_cam_ctlr_csi_config_t Csi::getDefaultConfig(uint32_t width, uint32_t height)
{
    esp_cam_ctlr_csi_config_t cfg = {};
    cfg.ctlr_id = 0;
    cfg.h_res = width;
    cfg.v_res = height;
    cfg.lane_bit_rate_mbps = 200;
    cfg.input_data_color_type = CAM_CTLR_COLOR_RAW8;
    cfg.output_data_color_type = CAM_CTLR_COLOR_RGB565;
    cfg.data_lane_num = 2;
    cfg.byte_swap_en = false;
    cfg.queue_items = 1;
    return cfg;
}

bool IRAM_ATTR Csi::onTransFinished(esp_cam_ctlr_handle_t handle, 
                                    esp_cam_ctlr_trans_t *trans, 
                                    void *data) {
        
    Csi *self = static_cast<Csi*>(data);
    self->flag = true;
    self->counter++;
    return false;
}

bool IRAM_ATTR Csi::onTransStarted(esp_cam_ctlr_handle_t handle,
                                   esp_cam_ctlr_trans_t *trans,
                                   void *data) {
    Csi *self = static_cast<Csi*>(data);
    if (self->flag == false) {
        trans->buffer = self->trans.buffer;
        trans->buflen = self->trans.buflen;
    }
    return false;
}