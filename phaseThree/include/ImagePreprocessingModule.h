#pragma once

#include <Arduino.h>
#include <tensorflow/lite/c/common.h>

namespace ImagePreprocessingModule
{
    constexpr bool SAVE_AI_DEBUG_IMAGE = true;

    struct PreprocessStats
    {
        uint32_t decodeMs;
        uint32_t preprocessMs;
        size_t rgbBufferBytes;
    };

    bool begin();
    bool isReady();

    bool preprocess(
        const uint8_t* jpegBuffer,
        size_t jpegLength,
        TfLiteTensor* inputTensor,
        PreprocessStats& stats
    );

    bool saveDebugPpmImage(
        const TfLiteTensor* inputTensor,
        const char* path = "/ai_debug/latest.ppm"
    );
}
