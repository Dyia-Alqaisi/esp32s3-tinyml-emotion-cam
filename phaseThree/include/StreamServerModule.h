#pragma once

#include <Arduino.h>

namespace StreamServerModule
{
    bool begin();

    bool isClientConnected();
    uint32_t streamedFrameCount();
}
