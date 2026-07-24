#pragma once

#include <Arduino.h>

namespace WebServerModule
{
    bool begin();
    void handleClient();

    uint32_t capturedFrameCount();
}
