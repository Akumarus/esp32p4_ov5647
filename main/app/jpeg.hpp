#pragma once

#include "driver/jpeg_encode.h"
#include <stdint.h>

class Jpeg {
public:
    Jpeg(uint32_t width, uint32_t height);
    ~Jpeg();
    void encode(uint8_t *inpdata, uint8_t **outData, uint32_t *outSize);

private:
    uint8_t *inpBufferData;
    uint8_t *outBufferData;
    uint32_t inpBufferSize;
    uint32_t outBufferSize;
    uint32_t width;
    uint32_t height;
    jpeg_encoder_handle_t handle;
};