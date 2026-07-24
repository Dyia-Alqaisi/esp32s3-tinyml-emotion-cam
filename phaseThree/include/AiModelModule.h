#pragma once

#include <Arduino.h>

#include <tensorflow/lite/c/common.h>

namespace AiModelModule
{
    // Loads the model from SD into PSRAM, initializes TFLite Micro,
    // validates tensors, and runs one neutral-gray test inference.
    bool begin();

    bool isReady();

    const char* lastError();

    size_t modelSizeBytes();
    size_t tensorArenaSizeBytes();
    size_t tensorArenaUsedBytes();

    uint32_t lastInferenceTimeUs();

    int lastPredictedClass();
    const char* lastPredictedLabel();

    // Phase 9B extensions
    TfLiteTensor* inputTensor();
    TfLiteTensor* outputTensor();

    bool invoke();
    int predictedClassFromOutput();
    const char* className(int classIndex);

    float dequantizedOutput(int classIndex);
    int8_t rawOutput(int classIndex);
}
