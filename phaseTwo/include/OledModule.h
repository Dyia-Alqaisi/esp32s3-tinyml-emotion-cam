#pragma once

#include <Arduino.h>
#include <IPAddress.h>

namespace OledModule
{
    bool begin();

    void showBoot();

    // Remains visible until the website is opened for the first time.
    void showNetworkReady(const IPAddress& ipAddress);

    // Website and control events.
    void showWebsiteConnected();
    void showLive();
    void showPaused();
    void showCapturing();
    void showCaptured();
    void showSaving();
    void showSaved(const char* filename, size_t imageBytes);

    // Phase 8A dataset collection events.
    void showDatasetCapturing(const char* label);
    void showDatasetSaved(const char* label, uint32_t imageIndex);
    void showDatasetError(const char* reason);

    void showActionError(const char* action);

    void showFatal(const char* component);

    // Handles timed transitions from action text to the small emotion screen.
    void update();
}
