#pragma once

#include <Arduino.h>

namespace DatasetModule
{
    enum class Label : uint8_t
    {
        Neutral = 0,
        Happy,
        Sad,
        Surprise,
        Invalid
    };

    bool begin();

    bool parseLabel(
        const String& labelText,
        Label& label
    );

    const char* labelName(Label label);
    const char* labelDisplayName(Label label);

    bool captureAndSave(
        Label label,
        String& savedPath,
        size_t& imageBytes,
        uint32_t& imageIndex,
        String& errorMessage
    );

    uint32_t imageCount(Label label);
    uint32_t nextImageIndex(Label label);

    String lastSavedPath();
    bool isReady();
}
