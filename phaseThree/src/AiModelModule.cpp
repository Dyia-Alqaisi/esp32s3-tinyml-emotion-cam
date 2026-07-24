#include "AiModelModule.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

#include <Arduino.h>
#include <esp_heap_caps.h>

#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/system_setup.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/c/common.h>

#include "AiConfig.h"
#include "ModelOps.h"
#include "SdCardModule.h"

namespace
{
    struct AlignedPsramBuffer
    {
        void* raw = nullptr;
        uint8_t* aligned = nullptr;
        size_t size = 0;
    };

    AlignedPsramBuffer modelBuffer;
    AlignedPsramBuffer tensorArena;

    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* g_inputTensor = nullptr;
    TfLiteTensor* g_outputTensor = nullptr;

    alignas(tflite::MicroInterpreter)
    uint8_t interpreterStorage[
        sizeof(tflite::MicroInterpreter)
    ];

    bool ready = false;
    char errorMessage[64] = "NOT INITIALIZED";

    size_t modelBytes = 0;
    size_t arenaUsedBytes = 0;
    uint32_t inferenceTimeUs = 0;
    int predictedClass = -1;

    void setError(const char* message)
    {
        ready = false;

        strncpy(
            errorMessage,
            message,
            sizeof(errorMessage) - 1
        );

        errorMessage[sizeof(errorMessage) - 1] = '\0';

        Serial.printf(
            "AI model error        : %s\n",
            errorMessage
        );
    }

    bool allocateAlignedPsram(
        size_t requestedBytes,
        size_t alignment,
        AlignedPsramBuffer& buffer
    )
    {
        if (
            requestedBytes == 0 ||
            alignment == 0 ||
            (alignment & (alignment - 1)) != 0
        ) {
            return false;
        }

        const size_t allocationBytes =
            requestedBytes + alignment - 1;

        buffer.raw = heap_caps_malloc(
            allocationBytes,
            MALLOC_CAP_SPIRAM |
            MALLOC_CAP_8BIT
        );

        if (buffer.raw == nullptr) {
            return false;
        }

        const uintptr_t rawAddress =
            reinterpret_cast<uintptr_t>(buffer.raw);

        const uintptr_t alignedAddress =
            (rawAddress + alignment - 1) &
            ~(static_cast<uintptr_t>(alignment - 1));

        buffer.aligned = reinterpret_cast<uint8_t*>(alignedAddress);
        buffer.size = requestedBytes;

        memset(buffer.aligned, 0, requestedBytes);
        return true;
    }

    bool validateInputTensor()
    {
        if (
            g_inputTensor == nullptr ||
            g_inputTensor->type != kTfLiteInt8 ||
            g_inputTensor->dims == nullptr ||
            g_inputTensor->dims->size != 4
        ) {
            setError("BAD INPUT TENSOR");
            return false;
        }

        if (
            g_inputTensor->dims->data[0] != AiConfig::INPUT_BATCH ||
            g_inputTensor->dims->data[1] != AiConfig::INPUT_HEIGHT ||
            g_inputTensor->dims->data[2] != AiConfig::INPUT_WIDTH ||
            g_inputTensor->dims->data[3] != AiConfig::INPUT_CHANNELS
        ) {
            setError("INPUT SHAPE");
            return false;
        }

        const size_t expectedBytes =
            static_cast<size_t>(
                AiConfig::INPUT_BATCH *
                AiConfig::INPUT_HEIGHT *
                AiConfig::INPUT_WIDTH *
                AiConfig::INPUT_CHANNELS
            );

        if (g_inputTensor->bytes != expectedBytes) {
            setError("INPUT BYTES");
            return false;
        }

        if (
            fabsf(
                g_inputTensor->params.scale -
                AiConfig::EXPECTED_INPUT_SCALE
            ) > 0.0001f ||
            g_inputTensor->params.zero_point !=
                AiConfig::EXPECTED_INPUT_ZERO_POINT
        ) {
            setError("INPUT QUANT");
            return false;
        }

        return true;
    }

    bool validateOutputTensor()
    {
        if (
            g_outputTensor == nullptr ||
            g_outputTensor->type != kTfLiteInt8 ||
            g_outputTensor->dims == nullptr ||
            g_outputTensor->dims->size != 2
        ) {
            setError("BAD OUTPUT TENSOR");
            return false;
        }

        if (
            g_outputTensor->dims->data[0] != 1 ||
            g_outputTensor->dims->data[1] != AiConfig::OUTPUT_CLASSES ||
            g_outputTensor->bytes != AiConfig::OUTPUT_CLASSES
        ) {
            setError("OUTPUT SHAPE");
            return false;
        }

        return true;
    }

    void printTensorInformation()
    {
        Serial.println("----------------------------------------");
        Serial.println("AI tensor information");

        Serial.printf(
            "Input shape           : [%d,%d,%d,%d]\n",
            g_inputTensor->dims->data[0],
            g_inputTensor->dims->data[1],
            g_inputTensor->dims->data[2],
            g_inputTensor->dims->data[3]
        );

        Serial.printf("Input type            : INT8\n");
        Serial.printf(
            "Input bytes           : %u\n",
            static_cast<unsigned int>(g_inputTensor->bytes)
        );
        Serial.printf(
            "Input scale           : %.8f\n",
            g_inputTensor->params.scale
        );
        Serial.printf(
            "Input zero point      : %ld\n",
            static_cast<long>(g_inputTensor->params.zero_point)
        );

        Serial.printf(
            "Output shape          : [%d,%d]\n",
            g_outputTensor->dims->data[0],
            g_outputTensor->dims->data[1]
        );
        Serial.printf("Output type           : INT8 logits\n");
        Serial.printf(
            "Output scale          : %.8f\n",
            g_outputTensor->params.scale
        );
        Serial.printf(
            "Output zero point     : %ld\n",
            static_cast<long>(g_outputTensor->params.zero_point)
        );
    }

    bool runGrayTestInference()
    {
        memset(g_inputTensor->data.int8, 0, g_inputTensor->bytes);

        const uint32_t startedAt = micros();
        const TfLiteStatus status = interpreter->Invoke();
        inferenceTimeUs = micros() - startedAt;

        if (status != kTfLiteOk) {
            setError("INVOKE FAILED");
            return false;
        }

        predictedClass = 0;

        Serial.println("----------------------------------------");
        Serial.println("Gray-image test logits");

        for (int index = 0; index < AiConfig::OUTPUT_CLASSES; ++index) {
            const int8_t rawLogit = g_outputTensor->data.int8[index];
            const float dequantizedLogit =
                (static_cast<int32_t>(rawLogit) - g_outputTensor->params.zero_point) *
                g_outputTensor->params.scale;

            Serial.printf(
                "%-10s raw=%4d  logit=% .5f\n",
                AiConfig::CLASS_NAMES[index],
                static_cast<int>(rawLogit),
                dequantizedLogit
            );

            if (rawLogit > g_outputTensor->data.int8[predictedClass]) {
                predictedClass = index;
            }
        }

        Serial.printf(
            "Gray test prediction  : %s\n",
            AiConfig::CLASS_NAMES[predictedClass]
        );
        Serial.printf(
            "Inference time        : %lu us (%.2f ms)\n",
            static_cast<unsigned long>(inferenceTimeUs),
            inferenceTimeUs / 1000.0f
        );

        return true;
    }
}

namespace AiModelModule
{
    bool begin()
    {
        ready = false;
        predictedClass = -1;
        inferenceTimeUs = 0;
        arenaUsedBytes = 0;
        modelBytes = 0;

        Serial.println();
        Serial.println("========================================");
        Serial.println(" PHASE 9A - TFLITE MICRO INITIALIZATION");
        Serial.println("========================================");

        if (!psramFound()) {
            setError("PSRAM NOT FOUND");
            return false;
        }

        if (!SdCardModule::isMounted()) {
            setError("SD NOT MOUNTED");
            return false;
        }

        if (!SdCardModule::exists(AiConfig::MODEL_PATH)) {
            setError("MODEL NOT FOUND");
            Serial.printf("Expected path         : %s\n", AiConfig::MODEL_PATH);
            return false;
        }

        modelBytes = SdCardModule::fileSize(AiConfig::MODEL_PATH);

        if (modelBytes < 1024 || modelBytes > AiConfig::MAX_MODEL_BYTES) {
            setError("MODEL SIZE");
            return false;
        }

        Serial.printf("Model path            : %s\n", AiConfig::MODEL_PATH);
        Serial.printf(
            "Model size            : %u bytes (%.3f MiB)\n",
            static_cast<unsigned int>(modelBytes),
            modelBytes / (1024.0f * 1024.0f)
        );
        Serial.printf(
            "Free PSRAM before AI  : %u bytes\n",
            ESP.getFreePsram()
        );

        if (!allocateAlignedPsram(
                modelBytes,
                AiConfig::MEMORY_ALIGNMENT,
                modelBuffer
            )) {
            setError("MODEL PSRAM");
            return false;
        }

        if (!SdCardModule::readFile(
                AiConfig::MODEL_PATH,
                modelBuffer.aligned,
                modelBytes
            )) {
            setError("MODEL READ");
            return false;
        }

        model = tflite::GetModel(modelBuffer.aligned);
        if (model == nullptr) {
            setError("MODEL PARSE");
            return false;
        }

        if (model->version() != TFLITE_SCHEMA_VERSION) {
            Serial.printf("Model schema          : %d\n", model->version());
            Serial.printf("Runtime schema        : %d\n", TFLITE_SCHEMA_VERSION);
            setError("SCHEMA VERSION");
            return false;
        }

        if (!allocateAlignedPsram(
                AiConfig::TENSOR_ARENA_BYTES,
                AiConfig::MEMORY_ALIGNMENT,
                tensorArena
            )) {
            setError("ARENA PSRAM");
            return false;
        }

        static ModelOps::Resolver resolver = ModelOps::createResolver();

        interpreter = new (interpreterStorage) tflite::MicroInterpreter(
            model,
            resolver,
            tensorArena.aligned,
            tensorArena.size
        );

        if (interpreter == nullptr) {
            setError("INTERPRETER");
            return false;
        }

        Serial.printf(
            "Tensor arena reserved : %u bytes (%.2f MiB)\n",
            static_cast<unsigned int>(tensorArena.size),
            tensorArena.size / (1024.0f * 1024.0f)
        );

        const TfLiteStatus allocationStatus = interpreter->AllocateTensors();
        if (allocationStatus != kTfLiteOk) {
            setError("ALLOCATE TENSORS");
            Serial.println("Possible causes: missing ModelOps entry or tensor arena too small.");
            return false;
        }

        arenaUsedBytes = interpreter->arena_used_bytes();
        g_inputTensor = interpreter->input(0);
        g_outputTensor = interpreter->output(0);

        if (!validateInputTensor()) {
            return false;
        }

        if (!validateOutputTensor()) {
            return false;
        }

        printTensorInformation();

        Serial.printf(
            "Tensor arena used     : %u bytes (%.2f MiB)\n",
            static_cast<unsigned int>(arenaUsedBytes),
            arenaUsedBytes / (1024.0f * 1024.0f)
        );
        Serial.printf(
            "Free PSRAM after AI   : %u bytes\n",
            ESP.getFreePsram()
        );

        if (!runGrayTestInference()) {
            return false;
        }

        ready = true;
        strncpy(errorMessage, "NONE", sizeof(errorMessage));

        Serial.println("----------------------------------------");
        Serial.println("AI MODEL MODULE       : PASS");
        Serial.println("Model resident        : PSRAM");
        Serial.println("Tensor arena          : PSRAM");
        Serial.println("Phase 9A result       : PASS");
        Serial.println("========================================");

        return true;
    }

    bool isReady()
    {
        return ready;
    }

    const char* lastError()
    {
        return errorMessage;
    }

    size_t modelSizeBytes()
    {
        return modelBytes;
    }

    size_t tensorArenaSizeBytes()
    {
        return tensorArena.size;
    }

    size_t tensorArenaUsedBytes()
    {
        return arenaUsedBytes;
    }

    uint32_t lastInferenceTimeUs()
    {
        return inferenceTimeUs;
    }

    int lastPredictedClass()
    {
        return predictedClass;
    }

    const char* lastPredictedLabel()
    {
        if (predictedClass < 0 || predictedClass >= AiConfig::OUTPUT_CLASSES) {
            return "unknown";
        }

        return AiConfig::CLASS_NAMES[predictedClass];
    }

    TfLiteTensor* inputTensor()
    {
        return g_inputTensor;
    }

    TfLiteTensor* outputTensor()
    {
        return g_outputTensor;
    }

    bool invoke()
    {
        if (!ready || interpreter == nullptr) {
            return false;
        }

        const uint32_t startedAt = micros();
        const TfLiteStatus status = interpreter->Invoke();
        inferenceTimeUs = micros() - startedAt;

        if (status != kTfLiteOk) {
            setError("INVOKE FAILED");
            return false;
        }

        predictedClass = predictedClassFromOutput();
        return true;
    }

    int predictedClassFromOutput()
    {
        if (g_outputTensor == nullptr || g_outputTensor->data.int8 == nullptr) {
            return -1;
        }

        int bestClass = 0;
        int8_t maxLogit = g_outputTensor->data.int8[0];

        for (int i = 1; i < AiConfig::OUTPUT_CLASSES; ++i) {
            if (g_outputTensor->data.int8[i] > maxLogit) {
                maxLogit = g_outputTensor->data.int8[i];
                bestClass = i;
            }
        }

        return bestClass;
    }

    const char* className(int classIndex)
    {
        if (classIndex < 0 || classIndex >= AiConfig::OUTPUT_CLASSES) {
            return "unknown";
        }
        return AiConfig::CLASS_NAMES[classIndex];
    }

    float dequantizedOutput(int classIndex)
    {
        if (g_outputTensor == nullptr || classIndex < 0 || classIndex >= AiConfig::OUTPUT_CLASSES) {
            return 0.0f;
        }

        const int8_t rawLogit = g_outputTensor->data.int8[classIndex];
        return (static_cast<int32_t>(rawLogit) - g_outputTensor->params.zero_point) *
               g_outputTensor->params.scale;
    }

    int8_t rawOutput(int classIndex)
    {
        if (g_outputTensor == nullptr || classIndex < 0 || classIndex >= AiConfig::OUTPUT_CLASSES) {
            return 0;
        }

        return g_outputTensor->data.int8[classIndex];
    }
}
