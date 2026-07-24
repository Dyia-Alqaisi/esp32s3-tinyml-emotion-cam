#pragma once

#include <Arduino.h>

namespace SdCardModule
{
    bool begin();

    bool saveJpeg(
        const uint8_t* imageData,
        size_t imageLength,
        String& savedPath
    );

    // Saves a verified JPEG to an explicit path without overwriting.
    bool saveJpegToPath(
        const uint8_t* imageData,
        size_t imageLength,
        const char* path
    );

    bool isMounted();
    uint64_t capacityMB();
}
