#include <Arduino.h>

#include "AppConfig.h"
#include "CameraModule.h"
#include "DatasetModule.h"
#include "OledModule.h"
#include "SdCardModule.h"
#include "StreamServerModule.h"
#include "WebServerModule.h"
#include "WifiModule.h"

namespace
{
    void stopWithFatalError(const char* component)
    {
        Serial.println();
        Serial.println("========================================");
        Serial.println("SYSTEM STARTUP FAILED");
        Serial.printf("Component: %s\n", component);
        Serial.println("========================================");

        OledModule::showFatal(component);

        while (true) {
            delay(1000);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(2500);

    Serial.println();
    Serial.println("========================================");
    Serial.println(" ALQAI EmotionCam - Modular Phase 8A");
    Serial.println("========================================");

    if (!OledModule::begin()) {
        stopWithFatalError("OLED");
    }

    OledModule::showBoot();

    if (!CameraModule::begin()) {
        stopWithFatalError("CAMERA");
    }

    if (!SdCardModule::begin()) {
        stopWithFatalError("SD CARD");
    }

    if (!DatasetModule::begin()) {
        stopWithFatalError("DATASET");
    }

    if (!WifiModule::begin()) {
        stopWithFatalError("WI-FI");
    }

    if (!WebServerModule::begin()) {
        stopWithFatalError("WEB");
    }

    if (!StreamServerModule::begin()) {
        stopWithFatalError("STREAM");
    }

    const IPAddress ipAddress =
        WifiModule::getIpAddress();

    OledModule::showNetworkReady(ipAddress);

    Serial.println("----------------------------------------");

    Serial.printf(
        "Website              : http://%s\n",
        ipAddress.toString().c_str()
    );

    Serial.printf(
        "Live stream          : http://%s:%u/stream\n",
        ipAddress.toString().c_str(),
        AppConfig::Http::STREAM_SERVER_PORT
    );

    Serial.printf(
        "Free heap            : %u bytes\n",
        ESP.getFreeHeap()
    );

    Serial.printf(
        "Free PSRAM           : %u bytes\n",
        ESP.getFreePsram()
    );

    Serial.println("----------------------------------------");
    Serial.println("PHASE 8A STARTED");
    Serial.println("Dataset collection is ready.");
    Serial.println("Connect to ALQAI-EmotionCam.");
    Serial.println("Open http://192.168.4.1");
    Serial.println("========================================");
}

void loop()
{
    WebServerModule::handleClient();
    OledModule::update();
    delay(2);
}
