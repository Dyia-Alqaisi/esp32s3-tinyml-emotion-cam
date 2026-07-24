#include "DatasetModule.h"

#include "CameraModule.h"
#include "DatasetConfig.h"
#include "OledModule.h"
#include "SdCardModule.h"

#include "FS.h"
#include "SD_MMC.h"
#include "esp_heap_caps.h"

namespace
{
    struct LabelState
    {
        DatasetModule::Label label;
        const char* name;
        const char* displayName;
        const char* directory;
        uint32_t imageCount;
        uint32_t nextIndex;
    };

    LabelState labelStates[DatasetConfig::LABEL_COUNT] = {
        {
            DatasetModule::Label::Neutral,
            "neutral",
            "NEUTRAL",
            "/dataset/neutral",
            0,
            1
        },
        {
            DatasetModule::Label::Happy,
            "happy",
            "HAPPY",
            "/dataset/happy",
            0,
            1
        },
        {
            DatasetModule::Label::Sad,
            "sad",
            "SAD",
            "/dataset/sad",
            0,
            1
        },
        {
            DatasetModule::Label::Surprise,
            "surprise",
            "SURPRISE",
            "/dataset/surprise",
            0,
            1
        }
    };

    bool datasetReady = false;
    String latestSavedPath;

    LabelState* findState(DatasetModule::Label label)
    {
        for (LabelState& state : labelStates) {
            if (state.label == label) {
                return &state;
            }
        }

        return nullptr;
    }

    const LabelState* findStateConst(
        DatasetModule::Label label
    )
    {
        for (const LabelState& state : labelStates) {
            if (state.label == label) {
                return &state;
            }
        }

        return nullptr;
    }

    bool ensureDirectory(const char* path)
    {
        if (SD_MMC.exists(path)) {
            File directory = SD_MMC.open(path, FILE_READ);

            const bool validDirectory =
                directory && directory.isDirectory();

            directory.close();
            return validDirectory;
        }

        return SD_MMC.mkdir(path);
    }

    bool parseImageIndex(
        const char* filePath,
        const char* labelName,
        uint32_t& imageIndex
    )
    {
        imageIndex = 0;

        if (
            filePath == nullptr ||
            labelName == nullptr
        ) {
            return false;
        }

        String fileName(filePath);
        const int slashPosition = fileName.lastIndexOf('/');

        if (slashPosition >= 0) {
            fileName = fileName.substring(slashPosition + 1);
        }

        const String prefix = String(labelName) + "_";
        constexpr char suffix[] = ".jpg";

        if (
            !fileName.startsWith(prefix) ||
            !fileName.endsWith(suffix)
        ) {
            return false;
        }

        const int numberStart = prefix.length();
        const int numberEnd =
            fileName.length() - (sizeof(suffix) - 1);

        if (numberEnd <= numberStart) {
            return false;
        }

        const String numberText =
            fileName.substring(numberStart, numberEnd);

        for (size_t i = 0; i < numberText.length(); ++i) {
            if (!isDigit(numberText.charAt(i))) {
                return false;
            }
        }

        const unsigned long parsedIndex =
            numberText.toInt();

        if (
            parsedIndex == 0 ||
            parsedIndex > DatasetConfig::MAX_IMAGE_INDEX
        ) {
            return false;
        }

        imageIndex = static_cast<uint32_t>(parsedIndex);
        return true;
    }

    bool scanLabelDirectory(LabelState& state)
    {
        state.imageCount = 0;
        state.nextIndex = 1;

        File directory = SD_MMC.open(
            state.directory,
            FILE_READ
        );

        if (!directory || !directory.isDirectory()) {
            directory.close();
            return false;
        }

        uint32_t highestIndex = 0;

        while (true) {
            File entry = directory.openNextFile();

            if (!entry) {
                break;
            }

            if (!entry.isDirectory()) {
                uint32_t imageIndex = 0;

                if (
                    parseImageIndex(
                        entry.name(),
                        state.name,
                        imageIndex
                    )
                ) {
                    state.imageCount++;

                    if (imageIndex > highestIndex) {
                        highestIndex = imageIndex;
                    }
                }
            }

            entry.close();
        }

        directory.close();

        state.nextIndex = highestIndex + 1;
        return true;
    }
}

namespace DatasetModule
{
    bool begin()
    {
        datasetReady = false;
        latestSavedPath = "";

        if (!SdCardModule::isMounted()) {
            Serial.println(
                "Dataset: SD card is not mounted."
            );

            return false;
        }

        if (!ensureDirectory(DatasetConfig::ROOT_DIRECTORY)) {
            Serial.println(
                "Dataset: could not create /dataset."
            );

            return false;
        }

        for (LabelState& state : labelStates) {
            if (!ensureDirectory(state.directory)) {
                Serial.printf(
                    "Dataset: could not create %s.\n",
                    state.directory
                );

                return false;
            }

            if (!scanLabelDirectory(state)) {
                Serial.printf(
                    "Dataset: could not scan %s.\n",
                    state.directory
                );

                return false;
            }
        }

        datasetReady = true;

        Serial.println("Dataset module        : PASS");

        for (const LabelState& state : labelStates) {
            Serial.printf(
                "Dataset %-10s: %lu images | next=%04lu\n",
                state.name,
                static_cast<unsigned long>(state.imageCount),
                static_cast<unsigned long>(state.nextIndex)
            );
        }

        return true;
    }

    bool parseLabel(
        const String& labelText,
        Label& label
    )
    {
        String normalized = labelText;
        normalized.trim();
        normalized.toLowerCase();

        for (const LabelState& state : labelStates) {
            if (normalized == state.name) {
                label = state.label;
                return true;
            }
        }

        label = Label::Invalid;
        return false;
    }

    const char* labelName(Label label)
    {
        const LabelState* state = findStateConst(label);
        return state != nullptr ? state->name : "invalid";
    }

    const char* labelDisplayName(Label label)
    {
        const LabelState* state = findStateConst(label);
        return state != nullptr
            ? state->displayName
            : "INVALID";
    }

    bool captureAndSave(
        Label label,
        String& savedPath,
        size_t& imageBytes,
        uint32_t& imageIndex,
        String& errorMessage
    )
    {
        savedPath = "";
        imageBytes = 0;
        imageIndex = 0;
        errorMessage = "";

        LabelState* state = findState(label);

        if (!datasetReady || state == nullptr) {
            errorMessage = "Dataset module is not ready";
            OledModule::showDatasetError("NOT READY");
            return false;
        }

        if (
            state->nextIndex == 0 ||
            state->nextIndex > DatasetConfig::MAX_IMAGE_INDEX
        ) {
            errorMessage = "Maximum image index reached";
            OledModule::showDatasetError("INDEX FULL");
            return false;
        }

        OledModule::showDatasetCapturing(
            state->displayName
        );

        uint8_t* jpegBuffer = nullptr;
        size_t jpegCapacity = 0;
        size_t jpegLength = 0;

        const bool captured = CameraModule::captureJpeg(
            jpegBuffer,
            jpegCapacity,
            jpegLength
        );

        if (!captured) {
            if (jpegBuffer != nullptr) {
                heap_caps_free(jpegBuffer);
            }

            errorMessage = "Camera capture failed";
            OledModule::showDatasetError("CAMERA");
            return false;
        }

        char path[96];

        snprintf(
            path,
            sizeof(path),
            "%s/%s_%04lu.jpg",
            state->directory,
            state->name,
            static_cast<unsigned long>(state->nextIndex)
        );

        const bool saved = SdCardModule::saveJpegToPath(
            jpegBuffer,
            jpegLength,
            path
        );

        heap_caps_free(jpegBuffer);

        if (!saved) {
            errorMessage = "SD write or JPEG verification failed";
            OledModule::showDatasetError("SD WRITE");
            return false;
        }

        imageBytes = jpegLength;
        imageIndex = state->nextIndex;
        savedPath = path;
        latestSavedPath = path;

        state->imageCount++;
        state->nextIndex++;

        OledModule::showDatasetSaved(
            state->displayName,
            imageIndex
        );

        Serial.printf(
            "Dataset image saved: %s | %u bytes | "
            "heap=%u | PSRAM=%u\n",
            path,
            static_cast<unsigned int>(imageBytes),
            ESP.getFreeHeap(),
            ESP.getFreePsram()
        );

        return true;
    }

    uint32_t imageCount(Label label)
    {
        const LabelState* state = findStateConst(label);
        return state != nullptr ? state->imageCount : 0;
    }

    uint32_t nextImageIndex(Label label)
    {
        const LabelState* state = findStateConst(label);
        return state != nullptr ? state->nextIndex : 0;
    }

    String lastSavedPath()
    {
        return latestSavedPath;
    }

    bool isReady()
    {
        return datasetReady;
    }

    bool resetAll()
    {
        if (!datasetReady || !SdCardModule::isMounted()) {
            return false;
        }

        for (LabelState& state : labelStates) {
            File dir = SD_MMC.open(state.directory, FILE_READ);
            if (dir && dir.isDirectory()) {
                while (true) {
                    File entry = dir.openNextFile();
                    if (!entry) {
                        break;
                    }
                    if (!entry.isDirectory()) {
                        String fullPath = String(entry.name());
                        entry.close();
                        SD_MMC.remove(fullPath);
                    } else {
                        entry.close();
                    }
                }
            }
            if (dir) {
                dir.close();
            }

            state.imageCount = 0;
            state.nextIndex = 1;
        }

        latestSavedPath = "";
        Serial.println("Dataset: All images cleared and counts reset to 0.");
        return true;
    }
}
