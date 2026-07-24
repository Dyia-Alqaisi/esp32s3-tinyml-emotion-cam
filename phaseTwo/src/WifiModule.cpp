#include "WifiModule.h"

#include <WiFi.h>

#include "AppConfig.h"

namespace
{
    bool wifiReady = false;
}

namespace WifiModule
{
    bool begin()
    {
        wifiReady = false;

        WiFi.mode(WIFI_AP);
        WiFi.setSleep(false);

        if (
            !WiFi.softAP(
                AppConfig::Wifi::AP_SSID,
                AppConfig::Wifi::AP_PASSWORD
            )
        ) {
            Serial.println(
                "Wi-Fi: access point creation failed."
            );

            return false;
        }

        wifiReady = true;

        Serial.println("Wi-Fi module          : PASS");
        Serial.printf(
            "Wi-Fi SSID            : %s\n",
            AppConfig::Wifi::AP_SSID
        );

        Serial.printf(
            "Wi-Fi password        : %s\n",
            AppConfig::Wifi::AP_PASSWORD
        );

        Serial.printf(
            "Wi-Fi IP              : %s\n",
            getIpAddress().toString().c_str()
        );

        return true;
    }

    IPAddress getIpAddress()
    {
        return WiFi.softAPIP();
    }

    uint8_t connectedClientCount()
    {
        return WiFi.softAPgetStationNum();
    }

    bool isReady()
    {
        return wifiReady;
    }
}
