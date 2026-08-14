#include "bmp.hpp"
#include <cstring>

uint32_t Bmp::rowStride(uint32_t width)
{
    uint32_t rowBytes = width * BytesPerPixel;
    uint32_t padding = (4 - (rowBytes % 4)) % 4;
    return rowBytes + padding;
}

Bmp::Rgb888 Bmp::toRgb888(uint16_t px)
{
    Rgb888 c;
    c.r = ((px >> 11) & 0x1F) << 3;
    c.g = ((px >> 5) & 0x3F) << 2;
    c.b = (px & 0x1F) << 3;
    return c;
}

Bmp::FileHeader Bmp::makeFileHeader(uint32_t imgSize)
{
    FileHeader fh{};
    fh.type =  0x4D42;
    fh.size = sizeof(FileHeader) + sizeof(InfoHeader) + imgSize;
    fh.offset = sizeof(FileHeader) + sizeof(InfoHeader);
    return fh;
}

Bmp::InfoHeader Bmp::makeInfoHeader(uint32_t width, uint32_t height, uint32_t imgsize)
{
    InfoHeader ih{};
    ih.size = sizeof(InfoHeader);
    ih.width = width;
    ih.height = height;
    ih.planes = 1;
    ih.bitCount = 24;
    ih.compression = 0;
    ih.imageSize = imgsize;
    ih.xPelsPerMeter = 0;
    ih.yPelsPerMeter = 0;
    ih.clrUsed = 0;
    ih.clrImportant = 0;
    return ih;
}

void Bmp::writeRow(uint8_t *dst, const uint16_t *src, uint32_t width, uint32_t height, uint32_t stride)
{
        for (uint32_t y = 0; y < height; y++) {
        uint32_t srcY = height - 1 - y;
        uint8_t *dstRow = dst + y * stride;
        const uint16_t *srcRow = src + srcY * width;
        std::memset(dstRow, 0, stride);
        for (uint32_t x = 0; x < width; x++) {
            Rgb888 c = toRgb888(srcRow[x]);
            dstRow[x * BytesPerPixel + 0] = c.b;
            dstRow[x * BytesPerPixel + 1] = c.g;
            dstRow[x * BytesPerPixel + 2] = c.r;
        }
    }
}

uint32_t Bmp::encodedSize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0) {
        return 0;
    }
    uint32_t image_size = rowStride(width) * height;
    return sizeof(FileHeader) + sizeof(InfoHeader) + image_size;
}

bool Bmp::save(uint8_t *dst, uint32_t dstSize, const uint16_t *rgb565, uint32_t width, uint32_t height)
{
    if (!dst || !rgb565 || width == 0 || height == 0) {
        return false;
    }

    const uint32_t stride = rowStride(width);
    const uint32_t imageSize = stride * height;
    const uint32_t totalSize = sizeof(FileHeader) + sizeof(InfoHeader) + imageSize;

    if (dstSize < totalSize) {
        return false;
    }

    FileHeader fh = makeFileHeader(imageSize);
    InfoHeader ih = makeInfoHeader(width, height, imageSize);

    std::memcpy(dst, &fh, sizeof(fh));
    std::memcpy(dst + sizeof(fh), &ih, sizeof(ih));
    writeRow(dst + sizeof(fh) + sizeof(ih), rgb565, width, height, stride);

    return true;
}