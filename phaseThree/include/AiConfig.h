#pragma once

#include <Arduino.h>

namespace AiConfig
{
    // The model is loaded from the microSD card and remains resident in PSRAM.
    constexpr char MODEL_PATH[] =
        "/models/emotioncam_model.tflite";

    constexpr size_t MAX_MODEL_BYTES =
        4U * 1024U * 1024U;

    // Intentionally generous for the first allocation test.
    // After Phase 9A passes, use arena_used_bytes() to reduce this safely.
    constexpr size_t TENSOR_ARENA_BYTES =
        3U * 1024U * 1024U;

    constexpr size_t MEMORY_ALIGNMENT = 16;

    constexpr int INPUT_BATCH = 1;
    constexpr int INPUT_HEIGHT = 128;
    constexpr int INPUT_WIDTH = 128;
    constexpr int INPUT_CHANNELS = 3;
    constexpr int OUTPUT_CLASSES = 4;

    constexpr float EXPECTED_INPUT_SCALE = 1.0f;
    constexpr int EXPECTED_INPUT_ZERO_POINT = -128;

    constexpr const char* CLASS_NAMES[OUTPUT_CLASSES] = {
        "neutral",
        "happy",
        "sad",
        "surprise"
    };
}
