#include "ImagePreprocessingModule.h"

#include <cmath>
#include <cstring>

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <esp_heap_caps.h>

#include "img_converters.h"
#include "SdCardModule.h"

namespace
{
    constexpr size_t IMAGE_WIDTH = 640;
    constexpr size_t IMAGE_HEIGHT = 480;
    constexpr size_t RGB888_BUFFER_BYTES = IMAGE_WIDTH * IMAGE_HEIGHT * 3; // 921,600 bytes

    uint8_t* rgbBuffer = nullptr;
    bool ready = false;
}

namespace ImagePreprocessingModule
{
    bool begin()
    {
        ready = false;

        if (rgbBuffer == nullptr) {
            rgbBuffer = static_cast<uint8_t*>(
                heap_caps_malloc(
                    RGB888_BUFFER_BYTES,
                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
                )
            );
        }

        if (rgbBuffer == nullptr) {
            Serial.println("ImagePreprocessing: PSRAM RGB888 buffer allocation failed.");
            return false;
        }

        memset(rgbBuffer, 0, RGB888_BUFFER_BYTES);
        ready = true;

        Serial.println("ImagePreprocessing    : PASS");
        Serial.printf(
            "RGB888 PSRAM buffer  : %u bytes (%.2f MiB)\n",
            static_cast<unsigned int>(RGB888_BUFFER_BYTES),
            RGB888_BUFFER_BYTES / (1024.0f * 1024.0f)
        );

        return true;
    }

    bool isReady()
    {
        return ready;
    }

    bool preprocess(
        const uint8_t* jpegBuffer,
        size_t jpegLength,
        TfLiteTensor* inputTensor,
        PreprocessStats& stats
    )
    {
        stats.decodeMs = 0;
        stats.preprocessMs = 0;
        stats.rgbBufferBytes = RGB888_BUFFER_BYTES;

        if (!ready || rgbBuffer == nullptr || jpegBuffer == nullptr || jpegLength == 0 || inputTensor == nullptr) {
            return false;
        }

        // Step 1: Decode JPEG to RGB888 in PSRAM
        const uint32_t decodeStart = millis();
        const bool decodeOk = fmt2rgb888(jpegBuffer, jpegLength, PIXFORMAT_JPEG, rgbBuffer);
        stats.decodeMs = millis() - decodeStart;

        if (!decodeOk) {
            Serial.println("ImagePreprocessing: JPEG to RGB888 decode failed.");
            return false;
        }

        // Step 2: Exact Center Crop (400x400 at 120,40), Bilinear Resize (128x128), and INT8 Quantization (pixel - 128)
        const uint32_t prepStart = millis();

        constexpr int CROP_X = 120;
        constexpr int CROP_Y = 40;
        constexpr int CROP_SIZE = 400;

        constexpr int DST_W = 128;
        constexpr int DST_H = 128;

        constexpr float SCALE_X = static_cast<float>(CROP_SIZE) / static_cast<float>(DST_W);
        constexpr float SCALE_Y = static_cast<float>(CROP_SIZE) / static_cast<float>(DST_H);

        int8_t* inputData = inputTensor->data.int8;

        for (int dy = 0; dy < DST_H; ++dy) {
            const float srcY = CROP_Y + ((dy + 0.5f) * SCALE_Y) - 0.5f;
            int y0 = static_cast<int>(floorf(srcY));
            int y1 = y0 + 1;
            const float fy = srcY - static_cast<float>(y0);

            if (y0 < CROP_Y) { y0 = CROP_Y; }
            if (y1 >= CROP_Y + CROP_SIZE) { y1 = CROP_Y + CROP_SIZE - 1; }

            for (int dx = 0; dx < DST_W; ++dx) {
                const float srcX = CROP_X + ((dx + 0.5f) * SCALE_X) - 0.5f;
                int x0 = static_cast<int>(floorf(srcX));
                int x1 = x0 + 1;
                const float fx = srcX - static_cast<float>(x0);

                if (x0 < CROP_X) { x0 = CROP_X; }
                if (x1 >= CROP_X + CROP_SIZE) { x1 = CROP_X + CROP_SIZE - 1; }

                const uint8_t* p00 = &rgbBuffer[(y0 * IMAGE_WIDTH + x0) * 3];
                const uint8_t* p10 = &rgbBuffer[(y0 * IMAGE_WIDTH + x1) * 3];
                const uint8_t* p01 = &rgbBuffer[(y1 * IMAGE_WIDTH + x0) * 3];
                const uint8_t* p11 = &rgbBuffer[(y1 * IMAGE_WIDTH + x1) * 3];

                const float w00 = (1.0f - fx) * (1.0f - fy);
                const float w10 = fx * (1.0f - fy);
                const float w01 = (1.0f - fx) * fy;
                const float w11 = fx * fy;

                const int outIdx = (dy * DST_W + dx) * 3;

                for (int c = 0; c < 3; ++c) {
                    const float interpolated = p00[c] * w00 + p10[c] * w10 + p01[c] * w01 + p11[c] * w11;
                    const float quantizedF = roundf(interpolated) - 128.0f;
                    int val = static_cast<int>(quantizedF);
                    if (val < -128) { val = -128; }
                    if (val > 127) { val = 127; }
                    inputData[outIdx + c] = static_cast<int8_t>(val);
                }
            }
        }

        stats.preprocessMs = millis() - prepStart;
        return true;
    }

    bool saveDebugPpmImage(
        const TfLiteTensor* inputTensor,
        const char* path
    )
    {
        if (!SdCardModule::isMounted() || inputTensor == nullptr || inputTensor->data.int8 == nullptr) {
            return false;
        }

        if (!SD_MMC.exists("/ai_debug")) {
            SD_MMC.mkdir("/ai_debug");
        }

        File file = SD_MMC.open(path, FILE_WRITE);
        if (!file) {
            Serial.printf("ImagePreprocessing: Could not open %s for debug PPM write.\n", path);
            return false;
        }

        char header[64];
        const int headerLen = snprintf(header, sizeof(header), "P6\n128 128\n255\n");
        file.write(reinterpret_cast<const uint8_t*>(header), headerLen);

        // Convert INT8 tensor back to uint8_t RGB for PPM visualization
        const int8_t* inputData = inputTensor->data.int8;
        uint8_t rgbPixel[3];

        for (size_t i = 0; i < 128 * 128; ++i) {
            for (int c = 0; c < 3; ++c) {
                const int val = static_cast<int>(inputData[i * 3 + c]) + 128;
                rgbPixel[c] = static_cast<uint8_t>(val < 0 ? 0 : (val > 255 ? 255 : val));
            }
            file.write(rgbPixel, 3);
        }

        file.flush();
        file.close();

        Serial.printf("ImagePreprocessing: Debug PPM saved to %s\n", path);
        return true;
    }
}
