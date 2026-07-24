#include "EmotionInferenceModule.h"

#include <cstring>

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "AiConfig.h"
#include "AiModelModule.h"
#include "CameraModule.h"
#include "ImagePreprocessingModule.h"
#include "OledModule.h"

namespace
{
    EmotionInferenceModule::Status currentStatus = {
        false,                               // ready
        false,                               // busy
        EmotionInferenceModule::State::Idle, // state
        "idle",                              // stateName
        -1,                                  // predictionIndex
        "none",                              // predictionLabel
        {0, 0, 0, 0},                        // rawLogits
        {0.0f, 0.0f, 0.0f, 0.0f},            // logits
        0, 0, 0, 0, 0,                       // timings
        0, 0, 0,                             // memory/bytes
        "NONE"                               // lastError
    };

    uint8_t* persistentJpegBuffer = nullptr;
    size_t persistentJpegCapacity = 0;
    TaskHandle_t workerTaskHandle = nullptr;

    const char* stateToName(EmotionInferenceModule::State state)
    {
        switch (state) {
            case EmotionInferenceModule::State::Idle:       return "idle";
            case EmotionInferenceModule::State::Capturing:  return "capturing";
            case EmotionInferenceModule::State::Processing: return "processing";
            case EmotionInferenceModule::State::Inference:  return "inference";
            case EmotionInferenceModule::State::Complete:   return "complete";
            case EmotionInferenceModule::State::Error:      return "error";
            default:                                        return "unknown";
        }
    }

    void setError(const char* message)
    {
        currentStatus.busy = false;
        currentStatus.state = EmotionInferenceModule::State::Error;
        currentStatus.stateName = stateToName(currentStatus.state);
        strncpy(currentStatus.lastError, message, sizeof(currentStatus.lastError) - 1);
        currentStatus.lastError[sizeof(currentStatus.lastError) - 1] = '\0';
    }

    void aiWorkerTask(void* parameter)
    {
        (void)parameter;

        // 1. CAPTURE
        currentStatus.state = EmotionInferenceModule::State::Capturing;
        currentStatus.stateName = stateToName(currentStatus.state);
        OledModule::showAiCapture();

        size_t jpegLength = 0;
        size_t width = 0;
        size_t height = 0;

        const uint32_t captureStart = millis();
        const bool captureOk = CameraModule::captureJpegCopy(
            persistentJpegBuffer,
            persistentJpegCapacity,
            jpegLength,
            width,
            height,
            2000
        );
        currentStatus.captureMs = millis() - captureStart;

        if (!captureOk) {
            Serial.println("AI Task Error: Camera frame capture copy failed.");
            setError("CAMERA_CAPTURE_FAILED");
            OledModule::showActionError("CAMERA");
            workerTaskHandle = nullptr;
            vTaskDelete(nullptr);
            return;
        }

        currentStatus.jpegBytes = jpegLength;

        // 2. PREPROCESSING
        currentStatus.state = EmotionInferenceModule::State::Processing;
        currentStatus.stateName = stateToName(currentStatus.state);
        OledModule::showAiProcessing("DECODING...");

        ImagePreprocessingModule::PreprocessStats prepStats = {};
        const bool prepOk = ImagePreprocessingModule::preprocess(
            persistentJpegBuffer,
            jpegLength,
            AiModelModule::inputTensor(),
            prepStats
        );

        currentStatus.decodeMs = prepStats.decodeMs;
        currentStatus.preprocessMs = prepStats.preprocessMs;

        if (!prepOk) {
            Serial.println("AI Task Error: Image preprocessing failed.");
            setError("PREPROCESS_FAILED");
            OledModule::showActionError("DECODE");
            workerTaskHandle = nullptr;
            vTaskDelete(nullptr);
            return;
        }

        if (ImagePreprocessingModule::SAVE_AI_DEBUG_IMAGE) {
            ImagePreprocessingModule::saveDebugPpmImage(AiModelModule::inputTensor());
        }

        // 3. INFERENCE
        currentStatus.state = EmotionInferenceModule::State::Inference;
        currentStatus.stateName = stateToName(currentStatus.state);
        OledModule::showAiInference();

        const uint32_t inferStart = millis();
        const bool invokeOk = AiModelModule::invoke();
        currentStatus.inferenceMs = millis() - inferStart;

        if (!invokeOk) {
            Serial.println("AI Task Error: Model invocation failed.");
            setError("INFERENCE_FAILED");
            OledModule::showActionError("INFER");
            workerTaskHandle = nullptr;
            vTaskDelete(nullptr);
            return;
        }

        // 4. PARSE RESULTS & METRICS
        currentStatus.predictionIndex = AiModelModule::predictedClassFromOutput();
        currentStatus.predictionLabel = AiModelModule::className(currentStatus.predictionIndex);

        for (int i = 0; i < AiConfig::OUTPUT_CLASSES; ++i) {
            currentStatus.rawLogits[i] = AiModelModule::rawOutput(i);
            currentStatus.logits[i] = AiModelModule::dequantizedOutput(i);
        }

        currentStatus.totalMs = currentStatus.captureMs + currentStatus.decodeMs +
                                currentStatus.preprocessMs + currentStatus.inferenceMs;
        currentStatus.freeHeap = ESP.getFreeHeap();
        currentStatus.freePsram = ESP.getFreePsram();

        // Serial Output matching Phase 9B format
        Serial.println("----------------------------------------");
        Serial.println("PHASE 9B - REAL CAMERA INFERENCE");
        Serial.printf("AI state              : COMPLETED\n");
        Serial.printf("JPEG size             : %u bytes\n", static_cast<unsigned int>(jpegLength));
        Serial.printf("Camera frame          : %ux%u JPEG\n", static_cast<unsigned int>(width), static_cast<unsigned int>(height));
        Serial.printf("Capture/copy time     : %u ms\n", currentStatus.captureMs);
        Serial.println("Frame returned        : YES");
        Serial.println("Camera mutex released : YES");
        Serial.println();
        Serial.println("Decode result          : PASS");
        Serial.printf("Decode time            : %u ms\n", currentStatus.decodeMs);
        Serial.println("Crop                   : x=120 y=40 size=400x400");
        Serial.println("Resize                 : 128x128 RGB");
        Serial.printf("Preprocess time        : %u ms\n", currentStatus.preprocessMs);
        Serial.println();
        Serial.println("Input tensor           : INT8 [1,128,128,3]");
        Serial.println("Invoke result          : PASS");
        Serial.printf("Inference time         : %u ms (%.2f s)\n", currentStatus.inferenceMs, currentStatus.inferenceMs / 1000.0f);
        Serial.println();

        for (int i = 0; i < AiConfig::OUTPUT_CLASSES; ++i) {
            Serial.printf(
                "%-10s raw=%4d  logit=% .5f\n",
                AiConfig::CLASS_NAMES[i],
                currentStatus.rawLogits[i],
                currentStatus.logits[i]
            );
        }

        Serial.println();
        Serial.printf("Prediction             : %s\n", currentStatus.predictionLabel);
        Serial.printf("Total AI time          : %u ms (%.2f s)\n", currentStatus.totalMs, currentStatus.totalMs / 1000.0f);
        Serial.printf("Free heap              : %u bytes\n", currentStatus.freeHeap);
        Serial.printf("Free PSRAM             : %u bytes\n", currentStatus.freePsram);
        Serial.println("PHASE 9B RESULT        : PASS");
        Serial.println("----------------------------------------");

        // 5. UPDATE OLED & STATE
        OledModule::showAiResult(currentStatus.predictionLabel, currentStatus.totalMs);

        strncpy(currentStatus.lastError, "NONE", sizeof(currentStatus.lastError));
        currentStatus.state = EmotionInferenceModule::State::Complete;
        currentStatus.stateName = stateToName(currentStatus.state);
        currentStatus.busy = false;

        workerTaskHandle = nullptr;
        vTaskDelete(nullptr);
    }
}

namespace EmotionInferenceModule
{
    bool begin()
    {
        currentStatus.ready = false;
        currentStatus.busy = false;

        if (!AiModelModule::isReady()) {
            setError("MODEL_NOT_READY");
            return false;
        }

        if (!CameraModule::isReady()) {
            setError("CAMERA_NOT_READY");
            return false;
        }

        if (!ImagePreprocessingModule::isReady()) {
            setError("PREPROCESS_NOT_READY");
            return false;
        }

        currentStatus.ready = true;
        currentStatus.state = State::Idle;
        currentStatus.stateName = stateToName(currentStatus.state);
        strncpy(currentStatus.lastError, "NONE", sizeof(currentStatus.lastError));

        Serial.println("EmotionInference      : PASS");
        return true;
    }

    bool isReady()
    {
        return currentStatus.ready;
    }

    bool isBusy()
    {
        return currentStatus.busy;
    }

    bool runAsync()
    {
        if (!currentStatus.ready || currentStatus.busy) {
            return false;
        }

        currentStatus.busy = true;
        currentStatus.state = State::Capturing;
        currentStatus.stateName = stateToName(currentStatus.state);

        const BaseType_t created = xTaskCreate(
            aiWorkerTask,
            "ai_worker_task",
            8192,
            nullptr,
            1,
            &workerTaskHandle
        );

        if (created != pdPASS) {
            Serial.println("EmotionInference: Could not create worker task.");
            setError("TASK_CREATE_FAILED");
            currentStatus.busy = false;
            return false;
        }

        return true;
    }

    Status getStatus()
    {
        return currentStatus;
    }
}
