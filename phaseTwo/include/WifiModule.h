#pragma once

#include <Arduino.h>
#include <IPAddress.h>

namespace WifiModule
{
    bool begin();

    IPAddress getIpAddress();
    uint8_t connectedClientCount();
    bool isReady();
}
