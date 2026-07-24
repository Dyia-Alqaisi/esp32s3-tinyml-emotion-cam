#include "WebServerModule.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "AppConfig.h"
#include "CameraModule.h"
#include "DatasetModule.h"
#include "EmotionInferenceModule.h"
#include "OledModule.h"
#include "SdCardModule.h"
#include "StreamServerModule.h"
#include "WebPage.h"
#include "WifiModule.h"

#include "esp_heap_caps.h"

namespace
{
    WebServer server(
        AppConfig::Http::WEB_SERVER_PORT
    );

    uint32_t capturedFrames = 0;

    void appendDatasetCounts(String& json)
    {
        json += "\"neutral\":";
        json += DatasetModule::imageCount(
            DatasetModule::Label::Neutral
        );
        json += ",";

        json += "\"happy\":";
        json += DatasetModule::imageCount(
            DatasetModule::Label::Happy
        );
        json += ",";

        json += "\"sad\":";
        json += DatasetModule::imageCount(
            DatasetModule::Label::Sad
        );
        json += ",";

        json += "\"surprise\":";
        json += DatasetModule::imageCount(
            DatasetModule::Label::Surprise
        );
    }

    void handleRoot()
    {
        // The Wi-Fi/SSID/IP screen remains visible until this route
        // confirms that the browser has opened the website.
        OledModule::showWebsiteConnected();

        server.send_P(
            200,
            "text/html",
            WebPage::INDEX_HTML
        );
    }

    void handleCapture()
    {
        OledModule::showCapturing();

        uint8_t* jpegBuffer = nullptr;
        size_t jpegCapacity = 0;
        size_t jpegLength = 0;

        const bool captured =
            CameraModule::captureJpeg(
                jpegBuffer,
                jpegCapacity,
                jpegLength,
                AppConfig::Camera::CAPTURE_TIMEOUT_MS
            );

        if (!captured) {
            if (jpegBuffer != nullptr) {
                heap_caps_free(jpegBuffer);
            }

            OledModule::showActionError("CAPTURE");

            server.send(
                500,
                "text/plain",
                "Camera capture failed"
            );

            return;
        }

        capturedFrames++;
        OledModule::showCaptured();

        WiFiClient client = server.client();

        client.printf(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: image/jpeg\r\n"
            "Content-Length: %u\r\n"
            "Cache-Control: no-store, no-cache, "
            "must-revalidate, max-age=0\r\n"
            "Pragma: no-cache\r\n"
            "Connection: close\r\n"
            "\r\n",
            static_cast<unsigned int>(jpegLength)
        );

        client.write(jpegBuffer, jpegLength);
        client.flush();

        heap_caps_free(jpegBuffer);
    }

    void handleStatus()
    {
        String json;
        json.reserve(360);

        json += "{";

        json += "\"camera\":";
        json += CameraModule::isReady()
            ? "true"
            : "false";
        json += ",";

        json += "\"sd\":";
        json += SdCardModule::isMounted()
            ? "true"
            : "false";
        json += ",";

        json += "\"dataset\":";
        json += DatasetModule::isReady()
            ? "true"
            : "false";
        json += ",";

        json += "\"stream\":";
        json += StreamServerModule::isClientConnected()
            ? "true"
            : "false";
        json += ",";

        json += "\"heap\":";
        json += ESP.getFreeHeap();
        json += ",";

        json += "\"psram\":";
        json += ESP.getFreePsram();
        json += ",";

        json += "\"capturedFrames\":";
        json += capturedFrames;
        json += ",";

        json += "\"streamedFrames\":";
        json += StreamServerModule::streamedFrameCount();
        json += ",";

        json += "\"clients\":";
        json += WifiModule::connectedClientCount();
        json += ",";

        json += "\"ip\":\"";
        json += WifiModule::getIpAddress().toString();
        json += "\"";

        json += "}";

        server.send(
            200,
            "application/json",
            json
        );
    }

    void handleUiEvent()
    {
        if (!server.hasArg("action")) {
            server.send(
                400,
                "application/json",
                "{\"success\":false,"
                "\"message\":\"Missing action\"}"
            );

            return;
        }

        const String action = server.arg("action");

        if (action == "pause") {
            OledModule::showPaused();
        } else if (action == "resume") {
            OledModule::showLive();
        } else {
            server.send(
                400,
                "application/json",
                "{\"success\":false,"
                "\"message\":\"Unknown action\"}"
            );

            return;
        }

        server.send(
            200,
            "application/json",
            "{\"success\":true}"
        );
    }

    void handleSave()
    {
        OledModule::showSaving();

        uint8_t* jpegBuffer = nullptr;
        size_t jpegCapacity = 0;
        size_t jpegLength = 0;

        const bool captured =
            CameraModule::captureJpeg(
                jpegBuffer,
                jpegCapacity,
                jpegLength,
                AppConfig::Camera::CAPTURE_TIMEOUT_MS
            );

        if (!captured) {
            if (jpegBuffer != nullptr) {
                heap_caps_free(jpegBuffer);
            }

            OledModule::showActionError("SAVE");

            server.send(
                500,
                "application/json",
                "{\"success\":false,"
                "\"message\":\"Camera capture failed\"}"
            );

            return;
        }

        String savedPath;

        const bool saved =
            SdCardModule::saveJpeg(
                jpegBuffer,
                jpegLength,
                savedPath
            );

        heap_caps_free(jpegBuffer);

        if (!saved) {
            OledModule::showActionError("SD CARD");

            server.send(
                500,
                "application/json",
                "{\"success\":false,"
                "\"message\":\"SD write or verification failed\"}"
            );

            return;
        }

        const int slashPosition =
            savedPath.lastIndexOf('/');

        const String shortName =
            slashPosition >= 0
                ? savedPath.substring(slashPosition + 1)
                : savedPath;

        OledModule::showSaved(
            shortName.c_str(),
            jpegLength
        );

        String json;
        json.reserve(180);

        json += "{";
        json += "\"success\":true,";
        json += "\"file\":\"";
        json += savedPath;
        json += "\",";
        json += "\"bytes\":";
        json += jpegLength;
        json += "}";

        server.send(
            200,
            "application/json",
            json
        );

        Serial.printf(
            "Web image saved: %s | %u bytes\n",
            savedPath.c_str(),
            static_cast<unsigned int>(jpegLength)
        );
    }

    void handleDatasetStatus()
    {
        String json;
        json.reserve(300);

        json += "{";
        json += "\"ready\":";
        json += DatasetModule::isReady()
            ? "true"
            : "false";
        json += ",";

        appendDatasetCounts(json);
        json += ",";

        json += "\"nextNeutral\":";
        json += DatasetModule::nextImageIndex(
            DatasetModule::Label::Neutral
        );
        json += ",";

        json += "\"nextHappy\":";
        json += DatasetModule::nextImageIndex(
            DatasetModule::Label::Happy
        );
        json += ",";

        json += "\"nextSad\":";
        json += DatasetModule::nextImageIndex(
            DatasetModule::Label::Sad
        );
        json += ",";

        json += "\"nextSurprise\":";
        json += DatasetModule::nextImageIndex(
            DatasetModule::Label::Surprise
        );
        json += ",";

        json += "\"lastFile\":\"";
        json += DatasetModule::lastSavedPath();
        json += "\"";
        json += "}";

        server.send(
            200,
            "application/json",
            json
        );
    }

    void handleDatasetCapture()
    {
        if (!server.hasArg("label")) {
            server.send(
                400,
                "application/json",
                "{\"success\":false,"
                "\"message\":\"Missing dataset label\"}"
            );

            return;
        }

        DatasetModule::Label label;

        if (!DatasetModule::parseLabel(
                server.arg("label"),
                label
            )) {
            server.send(
                400,
                "application/json",
                "{\"success\":false,"
                "\"message\":\"Invalid dataset label\"}"
            );

            return;
        }

        String savedPath;
        String errorMessage;
        size_t imageBytes = 0;
        uint32_t imageIndex = 0;

        const bool saved = DatasetModule::captureAndSave(
            label,
            savedPath,
            imageBytes,
            imageIndex,
            errorMessage
        );

        if (!saved) {
            String json;
            json.reserve(180);

            json += "{";
            json += "\"success\":false,";
            json += "\"message\":\"";
            json += errorMessage;
            json += "\"";
            json += "}";

            server.send(
                500,
                "application/json",
                json
            );

            return;
        }

        String json;
        json.reserve(320);

        json += "{";
        json += "\"success\":true,";
        json += "\"label\":\"";
        json += DatasetModule::labelName(label);
        json += "\",";
        json += "\"file\":\"";
        json += savedPath;
        json += "\",";
        json += "\"bytes\":";
        json += imageBytes;
        json += ",";
        json += "\"index\":";
        json += imageIndex;
        json += ",";
        appendDatasetCounts(json);
        json += "}";

        server.send(
            200,
            "application/json",
            json
        );
    }

    void handleDatasetReset()
    {
        if (!DatasetModule::resetAll()) {
            server.send(
                500,
                "application/json",
                "{\"success\":false,\"message\":\"Dataset reset failed\"}"
            );
            return;
        }

        String json;
        json.reserve(180);
        json += "{\"success\":true,";
        appendDatasetCounts(json);
        json += "}";

        server.send(
            200,
            "application/json",
            json
        );
    }

    void handleAiRun()
    {
        if (!EmotionInferenceModule::isReady()) {
            server.send(
                503,
                "application/json",
                "{\"accepted\":false,\"reason\":\"AI model not ready\"}"
            );
            return;
        }

        if (EmotionInferenceModule::isBusy()) {
            server.send(
                409,
                "application/json",
                "{\"accepted\":false,\"reason\":\"AI inference busy\"}"
            );
            return;
        }

        if (EmotionInferenceModule::runAsync()) {
            server.send(
                200,
                "application/json",
                "{\"accepted\":true,\"state\":\"queued\"}"
            );
        } else {
            server.send(
                500,
                "application/json",
                "{\"accepted\":false,\"reason\":\"Failed to start worker task\"}"
            );
        }
    }

    void handleAiStatus()
    {
        const EmotionInferenceModule::Status st = EmotionInferenceModule::getStatus();

        String json;
        json.reserve(380);

        json += "{";
        json += "\"ready\":";
        json += st.ready ? "true" : "false";
        json += ",";

        json += "\"busy\":";
        json += st.busy ? "true" : "false";
        json += ",";

        json += "\"state\":\"";
        json += st.stateName;
        json += "\",";

        json += "\"predictionIndex\":";
        json += st.predictionIndex;
        json += ",";

        json += "\"prediction\":\"";
        json += st.predictionLabel;
        json += "\",";

        json += "\"rawLogits\":[";
        json += st.rawLogits[0]; json += ",";
        json += st.rawLogits[1]; json += ",";
        json += st.rawLogits[2]; json += ",";
        json += st.rawLogits[3];
        json += "],";

        json += "\"logits\":[";
        json += String(st.logits[0], 2); json += ",";
        json += String(st.logits[1], 2); json += ",";
        json += String(st.logits[2], 2); json += ",";
        json += String(st.logits[3], 2);
        json += "],";

        json += "\"captureMs\":";
        json += st.captureMs;
        json += ",";

        json += "\"decodeMs\":";
        json += st.decodeMs;
        json += ",";

        json += "\"preprocessMs\":";
        json += st.preprocessMs;
        json += ",";

        json += "\"inferenceMs\":";
        json += st.inferenceMs;
        json += ",";

        json += "\"totalMs\":";
        json += st.totalMs;
        json += ",";

        json += "\"freeHeap\":";
        json += st.freeHeap;
        json += ",";

        json += "\"freePsram\":";
        json += st.freePsram;
        json += ",";

        json += "\"lastError\":\"";
        json += st.lastError;
        json += "\"";

        json += "}";

        server.send(
            200,
            "application/json",
            json
        );
    }
}

namespace WebServerModule
{
    bool begin()
    {
        server.on(
            "/",
            HTTP_GET,
            handleRoot
        );

        server.on(
            "/capture",
            HTTP_GET,
            handleCapture
        );

        server.on(
            "/status",
            HTTP_GET,
            handleStatus
        );

        server.on(
            "/ui-event",
            HTTP_GET,
            handleUiEvent
        );

        server.on(
            "/save",
            HTTP_GET,
            handleSave
        );

        server.on(
            "/dataset/status",
            HTTP_GET,
            handleDatasetStatus
        );

        server.on(
            "/dataset/capture",
            HTTP_POST,
            handleDatasetCapture
        );

        server.on(
            "/dataset/reset",
            HTTP_POST,
            handleDatasetReset
        );

        server.on(
            "/dataset/reset",
            HTTP_GET,
            handleDatasetReset
        );

        server.on(
            "/ai/run",
            HTTP_POST,
            handleAiRun
        );

        server.on(
            "/ai/run",
            HTTP_GET,
            handleAiRun
        );

        server.on(
            "/ai/status",
            HTTP_GET,
            handleAiStatus
        );

        server.on(
            "/favicon.ico",
            HTTP_GET,
            []() {
                server.send(
                    204,
                    "text/plain",
                    ""
                );
            }
        );

        server.onNotFound(
            []() {
                server.send(
                    404,
                    "text/plain",
                    "Not found"
                );
            }
        );

        server.begin();

        Serial.println("Web-server module     : PASS");

        return true;
    }

    void handleClient()
    {
        server.handleClient();
    }

    uint32_t capturedFrameCount()
    {
        return capturedFrames;
    }
}
