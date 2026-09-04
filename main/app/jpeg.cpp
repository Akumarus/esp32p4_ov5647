#include "jpeg.hpp"
#include "esp_log.h"

Jpeg::Jpeg(uint32_t width, uint32_t height) :
    inpBufferSize(width * height * 2),
    outBufferSize(width * height * 2),
    width(width), height(height)
{
    jpeg_encode_engine_cfg_t encoderConfig = {};
    encoderConfig.intr_priority = 0;
    encoderConfig.timeout_ms = 100;

    jpeg_new_encoder_engine(&encoderConfig, &handle);
    assert(handle);

    size_t allocatedSize = 0;
    jpeg_encode_memory_alloc_cfg_t memConfig = {};
    memConfig.buffer_direction = JPEG_ENC_ALLOC_INPUT_BUFFER;
    inpBufferData = static_cast<uint8_t*>(jpeg_alloc_encoder_mem(
        inpBufferSize, &memConfig, &allocatedSize));
    
    memConfig.buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER;
    outBufferData = static_cast<uint8_t*>(jpeg_alloc_encoder_mem(
        outBufferSize, &memConfig, &allocatedSize));

    ESP_LOGI("Jpeg", "Encoder initialized: %lux%lu", width, height);
}

void Jpeg::encode(uint8_t *inpData, uint8_t **outData, uint32_t *outSize)
{
    jpeg_encode_cfg_t config = {};
    config.src_type = JPEG_ENCODE_IN_FORMAT_RGB565,
    config.sub_sample = JPEG_DOWN_SAMPLING_YUV422,
    config.image_quality = 90;
    config.width = width;
    config.height = height;

    uint32_t size;
    jpeg_encoder_process(handle, &config, inpData, inpBufferSize, outBufferData, outBufferSize, &size);

    *outData = outBufferData;
    *outSize = size;
}