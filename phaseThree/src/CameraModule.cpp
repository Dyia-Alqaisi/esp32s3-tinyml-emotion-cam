#include "CameraModule.h"

#include <cstring>

#include "AppConfig.h"
#include "esp_camera.h"
#include "esp_err.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace
{
    SemaphoreHandle_t cameraMutex = nullptr;
    bool cameraReady = false;

    bool isValidJpegFrame(const camera_fb_t* frame)
    {
        if (
            frame == nullptr ||
            frame->buf == nullptr ||
            frame->len < 4
        ) {
            return false;
        }

        return (
            frame->buf[0] == 0xFF &&
            frame->buf[1] == 0xD8 &&
            frame->buf[frame->len - 2] == 0xFF &&
            frame->buf[frame->len - 1] == 0xD9
        );
    }

    size_t roundUpTo4K(size_t value)
    {
        return (value + 4095U) &
               ~static_cast<size_t>(4095U);
    }

    bool warmUpCamera()
    {
        for (
            size_t i = 0;
            i < AppConfig::Camera::WARMUP_FRAMES;
            ++i
        ) {
            camera_fb_t* frame = esp_camera_fb_get();

            if (frame == nullptr) {
                Serial.printf(
                    "Camera warm-up frame %u failed.\n",
                    static_cast<unsigned int>(i + 1)
                );

                return false;
            }

            esp_camera_fb_return(frame);
            delay(100);
        }

        return true;
    }
}

namespace CameraModule
{
    bool begin()
    {
        cameraReady = false;

        if (!psramFound()) {
            Serial.println("Camera: PSRAM was not detected.");
            return false;
        }

        cameraMutex = xSemaphoreCreateMutex();

        if (cameraMutex == nullptr) {
            Serial.println("Camera: mutex creation failed.");
            return false;
        }

        camera_config_t config = {};

        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer = LEDC_TIMER_0;

        config.pin_d0 = AppConfig::Camera::PIN_D0;
        config.pin_d1 = AppConfig::Camera::PIN_D1;
        config.pin_d2 = AppConfig::Camera::PIN_D2;
        config.pin_d3 = AppConfig::Camera::PIN_D3;
        config.pin_d4 = AppConfig::Camera::PIN_D4;
        config.pin_d5 = AppConfig::Camera::PIN_D5;
        config.pin_d6 = AppConfig::Camera::PIN_D6;
        config.pin_d7 = AppConfig::Camera::PIN_D7;

        config.pin_xclk = AppConfig::Camera::PIN_XCLK;
        config.pin_pclk = AppConfig::Camera::PIN_PCLK;
        config.pin_vsync = AppConfig::Camera::PIN_VSYNC;
        config.pin_href = AppConfig::Camera::PIN_HREF;

        config.pin_sccb_sda = AppConfig::Camera::PIN_SIOD;
        config.pin_sccb_scl = AppConfig::Camera::PIN_SIOC;

        config.pin_pwdn = AppConfig::Camera::PIN_PWDN;
        config.pin_reset = AppConfig::Camera::PIN_RESET;

        config.xclk_freq_hz =
            AppConfig::Camera::XCLK_FREQUENCY_HZ;

        config.pixel_format = PIXFORMAT_JPEG;
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 12;

        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

        const esp_err_t result = esp_camera_init(&config);

        if (result != ESP_OK) {
            Serial.printf(
                "Camera initialization failed: 0x%X - %s\n",
                static_cast<unsigned int>(result),
                esp_err_to_name(result)
            );

            return false;
        }

        sensor_t* sensor = esp_camera_sensor_get();

        if (sensor == nullptr) {
            Serial.println("Camera: no sensor detected.");
            esp_camera_deinit();
            return false;
        }

        const char* sensorName = "Unknown";
        if (sensor->id.PID == OV3660_PID) {
            sensorName = "OV3660";
        } else if (sensor->id.PID == OV2640_PID) {
            sensorName = "OV2640";
        } else {
            Serial.printf("Camera: unexpected sensor PID 0x%02X detected.\n", sensor->id.PID);
            esp_camera_deinit();
            return false;
        }

        if (!warmUpCamera()) {
            esp_camera_deinit();
            return false;
        }

        cameraReady = true;

        Serial.println("Camera module         : PASS");
        Serial.printf("Camera sensor         : %s\n", sensorName);
        Serial.println("Camera resolution     : 640x480 JPEG");
        Serial.println("Camera config         : fb_count=1, grab_mode=CAMERA_GRAB_WHEN_EMPTY");

        return true;
    }

    bool captureJpeg(
        uint8_t*& buffer,
        size_t& bufferCapacity,
        size_t& imageLength,
        uint32_t timeoutMs
    )
    {
        imageLength = 0;

        if (
            !cameraReady ||
            cameraMutex == nullptr
        ) {
            return false;
        }

        if (
            xSemaphoreTake(
                cameraMutex,
                pdMS_TO_TICKS(timeoutMs)
            ) != pdTRUE
        ) {
            Serial.println("Camera capture mutex timeout.");
            return false;
        }

        camera_fb_t* frame = esp_camera_fb_get();

        if (!isValidJpegFrame(frame)) {
            if (frame != nullptr) {
                esp_camera_fb_return(frame);
            }

            xSemaphoreGive(cameraMutex);
            return false;
        }

        if (frame->len > bufferCapacity) {
            const size_t newCapacity =
                roundUpTo4K(frame->len);

            uint8_t* resizedBuffer = nullptr;

            if (buffer == nullptr) {
                resizedBuffer = static_cast<uint8_t*>(
                    heap_caps_malloc(
                        newCapacity,
                        MALLOC_CAP_SPIRAM |
                        MALLOC_CAP_8BIT
                    )
                );
            } else {
                resizedBuffer = static_cast<uint8_t*>(
                    heap_caps_realloc(
                        buffer,
                        newCapacity,
                        MALLOC_CAP_SPIRAM |
                        MALLOC_CAP_8BIT
                    )
                );
            }

            if (resizedBuffer == nullptr) {
                Serial.println(
                    "Camera: PSRAM JPEG buffer allocation failed."
                );

                esp_camera_fb_return(frame);
                xSemaphoreGive(cameraMutex);
                return false;
            }

            buffer = resizedBuffer;
            bufferCapacity = newCapacity;
        }

        memcpy(
            buffer,
            frame->buf,
            frame->len
        );

        imageLength = frame->len;

        esp_camera_fb_return(frame);
        xSemaphoreGive(cameraMutex);

        return true;
    }

    bool captureJpegCopy(
        uint8_t*& destination,
        size_t& destinationCapacity,
        size_t& jpegLength,
        size_t& width,
        size_t& height,
        uint32_t timeoutMs
    )
    {
        jpegLength = 0;
        width = 0;
        height = 0;

        if (!cameraReady || cameraMutex == nullptr) {
            return false;
        }

        if (xSemaphoreTake(cameraMutex, pdMS_TO_TICKS(timeoutMs)) != pdTRUE) {
            Serial.println("Camera captureJpegCopy mutex timeout.");
            return false;
        }

        camera_fb_t* frame = esp_camera_fb_get();

        if (!isValidJpegFrame(frame) || frame->format != PIXFORMAT_JPEG) {
            if (frame != nullptr) {
                esp_camera_fb_return(frame);
            }
            xSemaphoreGive(cameraMutex);
            return false;
        }

        width = frame->width;
        height = frame->height;
        jpegLength = frame->len;

        if (jpegLength > destinationCapacity) {
            const size_t newCapacity = roundUpTo4K(jpegLength);
            uint8_t* newBuffer = nullptr;

            if (destination == nullptr) {
                newBuffer = static_cast<uint8_t*>(
                    heap_caps_malloc(newCapacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
                );
            } else {
                newBuffer = static_cast<uint8_t*>(
                    heap_caps_realloc(destination, newCapacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
                );
            }

            if (newBuffer == nullptr) {
                Serial.println("Camera: PSRAM destination allocation failed.");
                esp_camera_fb_return(frame);
                xSemaphoreGive(cameraMutex);
                return false;
            }

            destination = newBuffer;
            destinationCapacity = newCapacity;
        }

        memcpy(destination, frame->buf, jpegLength);

        // Frame and mutex released IMMEDIATELY
        esp_camera_fb_return(frame);
        xSemaphoreGive(cameraMutex);

        return true;
    }

    bool isReady()
    {
        return cameraReady;
    }
}
