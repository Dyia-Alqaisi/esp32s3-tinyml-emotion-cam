#pragma once

#include <Arduino.h>

namespace CameraModule
{
    bool begin();

    bool captureJpeg(
        uint8_t*& buffer,
        size_t& bufferCapacity,
        size_t& imageLength,
        uint32_t timeoutMs = 1500
    );

    bool isReady();
}
