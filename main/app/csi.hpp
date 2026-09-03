#pragma once

#include "esp_cam_ctlr_types.h"
#include "esp_cam_ctlr_csi.h"
#include "esp_cam_ctlr.h"
#include <cstdint>

class Csi {
public:
    Csi(esp_cam_ctlr_csi_config_t &config);
    ~Csi();

    bool enable();
    bool disable();
    bool start();
    bool stop();
    bool receive(uint32_t timeout);
    static esp_cam_ctlr_csi_config_t getDefaultConfig(uint32_t width, uint32_t height);
    esp_cam_ctlr_trans_t trans = {};
    volatile bool flag = false;
    volatile uint32_t counter = 0;

private:
    static bool onTransFinished(esp_cam_ctlr_handle_t handle, 
                                esp_cam_ctlr_trans_t *trans, void *data);
    static bool onTransStarted(esp_cam_ctlr_handle_t handle, 
                               esp_cam_ctlr_trans_t *trans, void *data);
    


    
    esp_cam_ctlr_handle_t handle = nullptr;
    esp_cam_ctlr_evt_cbs_t callbacks = {};
};