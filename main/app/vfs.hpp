#pragma once

#include "esp_vfs_fat.h"
#include "ldo.hpp"

class Vfs {
public:
    Vfs();
    ~Vfs();

    void writeFile(const char* path, uint8_t *data, uint32_t size);
private:
    Ldo ldo;
    sdmmc_host_t host;
    sdmmc_card_t *card = nullptr;
};