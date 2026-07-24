#pragma once

#include <Arduino.h>

namespace AppConfig
{
    namespace Camera
    {
        constexpr int PIN_PWDN = -1;
        constexpr int PIN_RESET = -1;

        constexpr int PIN_XCLK = 15;
        constexpr int PIN_SIOD = 4;
        constexpr int PIN_SIOC = 5;

        constexpr int PIN_D0 = 11;
        constexpr int PIN_D1 = 9;
        constexpr int PIN_D2 = 8;
        constexpr int PIN_D3 = 10;
        constexpr int PIN_D4 = 12;
        constexpr int PIN_D5 = 18;
        constexpr int PIN_D6 = 17;
        constexpr int PIN_D7 = 16;

        constexpr int PIN_VSYNC = 6;
        constexpr int PIN_HREF = 7;
        constexpr int PIN_PCLK = 13;

        constexpr uint32_t XCLK_FREQUENCY_HZ = 20000000;
        constexpr size_t WARMUP_FRAMES = 5;
        constexpr uint32_t CAPTURE_TIMEOUT_MS = 1500;
    }

    namespace Oled
    {
        constexpr int SDA = 42;
        constexpr int SCL = 41;
        constexpr uint8_t ADDRESS = 0x3C;
        constexpr int WIDTH = 128;
        constexpr int HEIGHT = 64;
        constexpr int RESET = -1;
        constexpr uint32_t I2C_FREQUENCY_HZ = 400000;
    }

    namespace SdCard
    {
        constexpr int CMD = 38;
        constexpr int CLK = 39;
        constexpr int D0 = 40;

        constexpr char MOUNT_POINT[] = "/sdcard";
        constexpr char CAPTURE_DIRECTORY[] = "/captures";
    }

    namespace Wifi
    {
        constexpr char AP_SSID[] = "ALQAI-EmotionCam";
        constexpr char AP_PASSWORD[] = "ALQAI360";
    }

    namespace Http
    {
        constexpr uint16_t WEB_SERVER_PORT = 80;
        constexpr uint16_t STREAM_SERVER_PORT = 81;
        constexpr uint16_t STREAM_CONTROL_PORT = 32769;
        constexpr uint32_t STREAM_FRAME_DELAY_MS = 50;
    }
}
