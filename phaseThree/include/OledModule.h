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

    // Phase 9A model-loading events.
    void showAiLoading();
    void showAiReady(size_t modelBytes, size_t arenaUsedBytes);
    void showAiError(const char* reason);

    // Phase 9B real-inference screens.
    void showAiCapture();
    void showAiProcessing(const char* detail);
    void showAiInference();
    void showAiResult(const char* label, uint32_t totalMs);

    // Phase 8A dataset collection events.
    void showDatasetCapturing(const char* label);
    void showDatasetSaved(const char* label, uint32_t imageIndex);
    void showDatasetError(const char* reason);

    void showActionError(const char* action);

    void showFatal(const char* component);

    // Handles timed transitions from action text to the small emotion screen.
    void update();
}
