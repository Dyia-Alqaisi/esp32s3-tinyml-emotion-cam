#include "StreamServerModule.h"

#include <cstring>

#include "AppConfig.h"
#include "CameraModule.h"

#include "esp_http_server.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace
{
    httpd_handle_t streamServer = nullptr;

    volatile bool streamClientConnected = false;
    volatile uint32_t streamedFrames = 0;

    const char* STREAM_CONTENT_TYPE =
        "multipart/x-mixed-replace;boundary="
        "123456789000000000000987654321";

    const char* STREAM_BOUNDARY =
        "\r\n--123456789000000000000987654321\r\n";

    const char* STREAM_PART_HEADER =
        "Content-Type: image/jpeg\r\n"
        "Content-Length: %u\r\n"
        "\r\n";

    esp_err_t handleStream(httpd_req_t* request)
    {
        if (streamClientConnected) {
            httpd_resp_set_status(
                request,
                "503 Service Unavailable"
            );

            httpd_resp_set_type(
                request,
                "text/plain"
            );

            return httpd_resp_send(
                request,
                "A live-stream client is already connected.",
                HTTPD_RESP_USE_STRLEN
            );
        }

        streamClientConnected = true;

        esp_err_t result = httpd_resp_set_type(
            request,
            STREAM_CONTENT_TYPE
        );

        if (result != ESP_OK) {
            streamClientConnected = false;
            return result;
        }

        httpd_resp_set_hdr(
            request,
            "Access-Control-Allow-Origin",
            "*"
        );

        httpd_resp_set_hdr(
            request,
            "Cache-Control",
            "no-store, no-cache, must-revalidate"
        );

        uint8_t* jpegBuffer = nullptr;
        size_t jpegCapacity = 0;
        size_t jpegLength = 0;

        uint8_t consecutiveFailures = 0;
        char partHeader[96];

        Serial.println("MJPEG client connected.");

        while (true) {
            const bool captured =
                CameraModule::captureJpeg(
                    jpegBuffer,
                    jpegCapacity,
                    jpegLength,
                    AppConfig::Camera::CAPTURE_TIMEOUT_MS
                );

            if (!captured) {
                consecutiveFailures++;

                if (consecutiveFailures >= 10) {
                    Serial.println(
                        "MJPEG stopped after repeated capture failures."
                    );

                    result = ESP_FAIL;
                    break;
                }

                vTaskDelay(pdMS_TO_TICKS(20));
                continue;
            }

            consecutiveFailures = 0;

            result = httpd_resp_send_chunk(
                request,
                STREAM_BOUNDARY,
                strlen(STREAM_BOUNDARY)
            );

            if (result != ESP_OK) {
                break;
            }

            const int headerLength = snprintf(
                partHeader,
                sizeof(partHeader),
                STREAM_PART_HEADER,
                static_cast<unsigned int>(jpegLength)
            );

            if (
                headerLength <= 0 ||
                headerLength >=
                    static_cast<int>(sizeof(partHeader))
            ) {
                result = ESP_FAIL;
                break;
            }

            result = httpd_resp_send_chunk(
                request,
                partHeader,
                static_cast<size_t>(headerLength)
            );

            if (result != ESP_OK) {
                break;
            }

            result = httpd_resp_send_chunk(
                request,
                reinterpret_cast<const char*>(jpegBuffer),
                jpegLength
            );

            if (result != ESP_OK) {
                break;
            }

            streamedFrames++;

            if ((streamedFrames % 100U) == 0U) {
                Serial.printf(
                    "MJPEG frames=%lu | JPEG=%u bytes | "
                    "heap=%u | PSRAM=%u\n",
                    static_cast<unsigned long>(streamedFrames),
                    static_cast<unsigned int>(jpegLength),
                    ESP.getFreeHeap(),
                    ESP.getFreePsram()
                );
            }

            vTaskDelay(
                pdMS_TO_TICKS(
                    AppConfig::Http::STREAM_FRAME_DELAY_MS
                )
            );
        }

        if (jpegBuffer != nullptr) {
            heap_caps_free(jpegBuffer);
        }

        streamClientConnected = false;

        Serial.println("MJPEG client disconnected.");

        return result;
    }
}

namespace StreamServerModule
{
    bool begin()
    {
        httpd_config_t config =
            HTTPD_DEFAULT_CONFIG();

        config.server_port =
            AppConfig::Http::STREAM_SERVER_PORT;

        config.ctrl_port =
            AppConfig::Http::STREAM_CONTROL_PORT;

        config.stack_size = 8192;
        config.max_open_sockets = 2;
        config.max_uri_handlers = 4;
        config.lru_purge_enable = true;

        const esp_err_t startResult =
            httpd_start(
                &streamServer,
                &config
            );

        if (startResult != ESP_OK) {
            Serial.printf(
                "MJPEG server start failed: 0x%X\n",
                static_cast<unsigned int>(startResult)
            );

            streamServer = nullptr;
            return false;
        }

        httpd_uri_t streamUri = {};

        streamUri.uri = "/stream";
        streamUri.method = HTTP_GET;
        streamUri.handler = handleStream;
        streamUri.user_ctx = nullptr;

        const esp_err_t routeResult =
            httpd_register_uri_handler(
                streamServer,
                &streamUri
            );

        if (routeResult != ESP_OK) {
            Serial.printf(
                "MJPEG route registration failed: 0x%X\n",
                static_cast<unsigned int>(routeResult)
            );

            httpd_stop(streamServer);
            streamServer = nullptr;

            return false;
        }

        Serial.println("MJPEG stream module   : PASS");

        return true;
    }

    bool isClientConnected()
    {
        return streamClientConnected;
    }

    uint32_t streamedFrameCount()
    {
        return streamedFrames;
    }
}
