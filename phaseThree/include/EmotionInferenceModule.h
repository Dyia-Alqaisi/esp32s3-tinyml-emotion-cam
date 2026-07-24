#pragma once

#include <Arduino.h>

namespace EmotionInferenceModule
{
    enum class State : uint8_t
    {
        Idle = 0,
        Capturing,
        Processing,
        Inference,
        Complete,
        Error
    };

    struct Status
    {
        bool ready;
        bool busy;
        State state;
        const char* stateName;
        int predictionIndex;
        const char* predictionLabel;
        int8_t rawLogits[4];
        float logits[4];
        uint32_t captureMs;
        uint32_t decodeMs;
        uint32_t preprocessMs;
        uint32_t inferenceMs;
        uint32_t totalMs;
        uint32_t freeHeap;
        uint32_t freePsram;
        size_t jpegBytes;
        char lastError[64];
    };

    bool begin();
    bool isReady();
    bool isBusy();

    bool runAsync();

    Status getStatus();
}
