#include "vfs.hpp"
#include "driver/sdmmc_host.h"
#include "esp_log.h"

Vfs::Vfs() : ldo(4)
{
    host = SDMMC_HOST_DEFAULT();
    host.pwr_ctrl_handle = ldo.as<sd_pwr_ctrl_handle_t>();
    host.max_freq_khz = 20000;

    sdmmc_slot_config_t slotConfig = SDMMC_SLOT_CONFIG_DEFAULT();
    slotConfig.width = 4;
    slotConfig.d0 = GPIO_NUM_39;
    slotConfig.d1 = GPIO_NUM_40;
    slotConfig.d2 = GPIO_NUM_41;
    slotConfig.d3 = GPIO_NUM_42;
    slotConfig.clk = GPIO_NUM_43;
    slotConfig.cmd = GPIO_NUM_44;
    slotConfig.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    
    esp_vfs_fat_sdmmc_mount_config_t mountConfig = {};
    mountConfig.format_if_mount_failed = false;
    mountConfig.max_files = 5;
    mountConfig.allocation_unit_size = 64 * 1024;

    ESP_ERROR_CHECK(esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slotConfig, &mountConfig, &card));
    
    ESP_LOGI("Main", "SD-карта смонтирована");
}

void Vfs::writeFile(const char* path, uint8_t *data, uint32_t size)
{
    FILE *file = fopen(path, "wb");
    if (file != nullptr) {
        fwrite(data, 1, size, file);
        fclose(file);
        ESP_LOGI("Vfs", "Файл сохранен");
    } else {
        ESP_LOGI("Vfs", "Ошибка открытия файла");
    }
}