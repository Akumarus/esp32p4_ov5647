#pragma once

#include <cstdint>

class Bmp 
{
public:
    static uint32_t encodedSize(uint32_t width, uint32_t height);
    static bool save(uint8_t *dst, uint32_t dstSize, const uint16_t *rgb565, uint32_t width, uint32_t height);

private:
    #pragma pack(push, 1)
    struct FileHeader
    {
        uint16_t type;
        uint32_t size;
        uint16_t rsv1;
        uint16_t rsv2;
        uint32_t offset;
    };
    
    struct InfoHeader
    {
        uint32_t size;
        int32_t width;
        int32_t height;
        uint16_t planes;
        uint16_t bitCount;
        uint32_t compression;
        uint32_t imageSize;
        int32_t xPelsPerMeter;
        int32_t yPelsPerMeter;
        uint32_t clrUsed;
        uint32_t clrImportant;
    };
    #pragma pack(pop)
    
    struct Rgb888
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
    };

private:
    static constexpr uint8_t BytesPerPixel = 3;

    static Rgb888 toRgb888(uint16_t px);
    static uint32_t rowStride(uint32_t width);
    static FileHeader makeFileHeader(uint32_t imgSize);
    static InfoHeader makeInfoHeader(uint32_t width, uint32_t height, uint32_t imgsize);
    static void writeRow(uint8_t *dst, const uint16_t *src, uint32_t width, uint32_t height, uint32_t stride);
};