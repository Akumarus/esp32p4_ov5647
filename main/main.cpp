#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_cache.h"
#include "ethernet.hpp"
#include "sensor.hpp"
#include "bmp.hpp"
#include "isp.hpp"
#include "csi.hpp"
#include "ldo.hpp"
#include "i2c.hpp"
#include "vfs.hpp"

extern "C" void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Sensor initialization */
    Sensor ov5647;
    ov5647.setFormat(800, 800);
    ov5647.enable();
    /* ISP initialization */
    auto ispConfig = Isp::getDefaultConfig(800, 800);
    Isp isp(ispConfig);
    /* Csi initialization */
    auto csiConfig = Csi::getDefaultConfig(800, 800);
    Csi csi(csiConfig);
    csi.enable();
    csi.start();
    csi.receive(ESP_CAM_CTLR_MAX_DELAY);
    /* Vfs initialization */
    Vfs vfs;
    /* Ethenet initialization */
    // Ethernet ethernet;

    while (csi.counter == 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGI("Main", "get frame");
    esp_cache_msync(csi.trans.buffer, csi.trans.buflen, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    uint32_t bmpSize = Bmp::encodedSize(800, 800);
    uint8_t *bmpData = (uint8_t *)heap_caps_aligned_alloc(8, bmpSize, MALLOC_CAP_SPIRAM);
    Bmp::save(bmpData, bmpSize, (uint16_t *)csi.trans.buffer, 800, 800);
    ESP_LOGI("Main", "frame converted to bmp");
    
    vfs.writeFile("/sdcard/frame.bmp", bmpData, bmpSize);

    while (true)
    {
        ESP_LOGI("Main", "frame count = %lu", csi.counter);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
