#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"

// =========================================================
// OLED configuration
// =========================================================

constexpr int OLED_SDA = 42;
constexpr int OLED_SCL = 41;

constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;
constexpr int OLED_RESET = -1;

Adafruit_SSD1306 display(
    OLED_WIDTH,
    OLED_HEIGHT,
    &Wire,
    OLED_RESET
);

// =========================================================
// OV3660 camera pins
// =========================================================

constexpr int CAMERA_PIN_PWDN  = -1;
constexpr int CAMERA_PIN_RESET = -1;

constexpr int CAMERA_PIN_XCLK = 15;
constexpr int CAMERA_PIN_SIOD = 4;
constexpr int CAMERA_PIN_SIOC = 5;

constexpr int CAMERA_PIN_D0 = 11;
constexpr int CAMERA_PIN_D1 = 9;
constexpr int CAMERA_PIN_D2 = 8;
constexpr int CAMERA_PIN_D3 = 10;
constexpr int CAMERA_PIN_D4 = 12;
constexpr int CAMERA_PIN_D5 = 18;
constexpr int CAMERA_PIN_D6 = 17;
constexpr int CAMERA_PIN_D7 = 16;

constexpr int CAMERA_PIN_VSYNC = 6;
constexpr int CAMERA_PIN_HREF  = 7;
constexpr int CAMERA_PIN_PCLK  = 13;

// =========================================================
// Built-in SD-card pins
// =========================================================

constexpr int SD_PIN_CMD = 38;
constexpr int SD_PIN_CLK = 39;
constexpr int SD_PIN_D0  = 40;

constexpr const char* SD_MOUNT_POINT = "/sdcard";

// =========================================================
// Integration-test configuration
// =========================================================

constexpr size_t WARMUP_FRAMES = 5;
constexpr size_t CAPTURE_COUNT = 3;

uint32_t lastHeartbeatTime = 0;

// =========================================================
// OLED utility functions
// =========================================================

void clearOled()
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
}

void drawCenteredText(
    const char* text,
    int y,
    uint8_t textSize
)
{
    int16_t x1;
    int16_t y1;
    uint16_t width;
    uint16_t height;

    display.setTextSize(textSize);

    display.getTextBounds(
        text,
        0,
        y,
        &x1,
        &y1,
        &width,
        &height
    );

    const int x =
        static_cast<int>((OLED_WIDTH - width) / 2);

    display.setCursor(x, y);
    display.print(text);
}

void showBootScreen()
{
    clearOled();

    drawCenteredText("ALQAI", 7, 2);

    display.drawLine(
        15,
        31,
        112,
        31,
        SSD1306_WHITE
    );

    drawCenteredText("EmotionCam", 39, 1);
    drawCenteredText("BOOTING...", 53, 1);

    display.display();
}

void showComponentStatus(
    const char* component,
    const char* status
)
{
    clearOled();

    drawCenteredText("ALQAI SYSTEM", 3, 1);

    display.drawLine(
        10,
        16,
        117,
        16,
        SSD1306_WHITE
    );

    drawCenteredText(component, 25, 2);
    drawCenteredText(status, 49, 1);

    display.display();
}

void showCaptureStatus(
    size_t currentImage,
    size_t totalImages
)
{
    char progressText[32];

    snprintf(
        progressText,
        sizeof(progressText),
        "IMAGE %u / %u",
        static_cast<unsigned int>(currentImage),
        static_cast<unsigned int>(totalImages)
    );

    clearOled();

    drawCenteredText("CAPTURING", 5, 2);
    drawCenteredText(progressText, 34, 1);

    // Progress bar border
    display.drawRect(
        13,
        49,
        102,
        11,
        SSD1306_WHITE
    );

    const int progressWidth =
        static_cast<int>(
            (98 * currentImage) / totalImages
        );

    display.fillRect(
        15,
        51,
        progressWidth,
        7,
        SSD1306_WHITE
    );

    display.display();
}

void showImageSaved(
    size_t imageNumber,
    size_t imageBytes
)
{
    char imageText[32];
    char sizeText[32];

    snprintf(
        imageText,
        sizeof(imageText),
        "IMAGE %u SAVED",
        static_cast<unsigned int>(imageNumber)
    );

    snprintf(
        sizeText,
        sizeof(sizeText),
        "%u BYTES",
        static_cast<unsigned int>(imageBytes)
    );

    clearOled();

    drawCenteredText("SD WRITE OK", 7, 1);
    drawCenteredText(imageText, 27, 1);
    drawCenteredText(sizeText, 45, 1);

    display.display();
}

void drawReadyFace()
{
    clearOled();

    // Left eye
    display.fillRoundRect(
        18,
        12,
        37,
        25,
        8,
        SSD1306_WHITE
    );

    // Right eye
    display.fillRoundRect(
        73,
        12,
        37,
        25,
        8,
        SSD1306_WHITE
    );

    // Pupils
    display.fillCircle(
        36,
        25,
        7,
        SSD1306_BLACK
    );

    display.fillCircle(
        91,
        25,
        7,
        SSD1306_BLACK
    );

    // Reflections
    display.fillCircle(
        34,
        22,
        2,
        SSD1306_WHITE
    );

    display.fillCircle(
        89,
        22,
        2,
        SSD1306_WHITE
    );

    // Smile
    display.drawLine(44, 46, 51, 52, SSD1306_WHITE);
    display.drawLine(51, 52, 58, 56, SSD1306_WHITE);
    display.drawLine(58, 56, 69, 56, SSD1306_WHITE);
    display.drawLine(69, 56, 77, 52, SSD1306_WHITE);
    display.drawLine(77, 52, 84, 46, SSD1306_WHITE);

    display.display();
}

void showFatalError(
    const char* component,
    const char* message
)
{
    Serial.println();
    Serial.println("========================================");
    Serial.println("PHASE 6 RESULT: FAILED");
    Serial.printf("%s: %s\n", component, message);
    Serial.println("========================================");

    clearOled();

    drawCenteredText("SYSTEM ERROR", 5, 1);
    drawCenteredText(component, 25, 2);
    drawCenteredText("CHECK SERIAL", 51, 1);

    display.display();

    while (true) {
        delay(1000);
    }
}

// =========================================================
// OLED initialization
// =========================================================

bool initializeOled()
{
    const bool wireStarted = Wire.begin(
        OLED_SDA,
        OLED_SCL,
        400000
    );

    if (!wireStarted) {
        Serial.println("Wire.begin() failed.");
        return false;
    }

    Wire.beginTransmission(OLED_ADDRESS);

    if (Wire.endTransmission() != 0) {
        Serial.println(
            "No OLED response at address 0x3C."
        );

        return false;
    }

    /*
     * periphBegin=false because Wire was already initialized
     * explicitly using GPIO42 and GPIO41.
     */
    const bool started = display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS,
        true,
        false
    );

    if (!started) {
        Serial.println("SSD1306 initialization failed.");
        return false;
    }

    display.dim(false);
    display.setTextWrap(false);

    return true;
}

// =========================================================
// Camera initialization
// =========================================================

bool initializeCamera()
{
    if (!psramFound()) {
        Serial.println("PSRAM was not detected.");
        return false;
    }

    camera_config_t config = {};

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    config.pin_d0 = CAMERA_PIN_D0;
    config.pin_d1 = CAMERA_PIN_D1;
    config.pin_d2 = CAMERA_PIN_D2;
    config.pin_d3 = CAMERA_PIN_D3;
    config.pin_d4 = CAMERA_PIN_D4;
    config.pin_d5 = CAMERA_PIN_D5;
    config.pin_d6 = CAMERA_PIN_D6;
    config.pin_d7 = CAMERA_PIN_D7;

    config.pin_xclk = CAMERA_PIN_XCLK;
    config.pin_pclk = CAMERA_PIN_PCLK;
    config.pin_vsync = CAMERA_PIN_VSYNC;
    config.pin_href = CAMERA_PIN_HREF;

    config.pin_sccb_sda = CAMERA_PIN_SIOD;
    config.pin_sccb_scl = CAMERA_PIN_SIOC;

    config.pin_pwdn = CAMERA_PIN_PWDN;
    config.pin_reset = CAMERA_PIN_RESET;

    config.xclk_freq_hz = 20000000;

    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;

    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

    const esp_err_t result =
        esp_camera_init(&config);

    if (result != ESP_OK) {
        Serial.printf(
            "Camera initialization error: 0x%X\n",
            static_cast<unsigned int>(result)
        );

        return false;
    }

    sensor_t* sensor = esp_camera_sensor_get();

    if (
        sensor == nullptr ||
        sensor->id.PID != OV3660_PID
    ) {
        Serial.println("OV3660 was not detected.");
        return false;
    }

    return true;
}

bool warmUpCamera()
{
    for (size_t i = 0; i < WARMUP_FRAMES; ++i) {
        camera_fb_t* frame = esp_camera_fb_get();

        if (frame == nullptr) {
            return false;
        }

        esp_camera_fb_return(frame);
        delay(100);
    }

    return true;
}

// =========================================================
// SD-card initialization
// =========================================================

bool initializeSdCard()
{
    if (!SD_MMC.setPins(
            SD_PIN_CLK,
            SD_PIN_CMD,
            SD_PIN_D0
        )) {
        Serial.println("SD_MMC.setPins() failed.");
        return false;
    }

    const bool mounted = SD_MMC.begin(
        SD_MOUNT_POINT,
        true,
        false,
        SDMMC_FREQ_DEFAULT,
        5
    );

    if (!mounted) {
        Serial.println("SD-card mount failed.");
        return false;
    }

    if (SD_MMC.cardType() == CARD_NONE) {
        Serial.println("No SD card detected.");
        return false;
    }

    if (!SD_MMC.exists("/captures")) {
        if (!SD_MMC.mkdir("/captures")) {
            Serial.println(
                "Could not create /captures."
            );

            return false;
        }
    }

    return true;
}

// =========================================================
// JPEG validation
// =========================================================

bool isValidJpeg(const camera_fb_t* frame)
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

bool verifySavedImage(
    const char* path,
    size_t expectedSize
)
{
    File file = SD_MMC.open(path, FILE_READ);

    if (!file) {
        return false;
    }

    if (file.size() != expectedSize) {
        file.close();
        return false;
    }

    uint8_t startMarker[2] = {};
    uint8_t endMarker[2] = {};

    const bool startRead =
        file.read(startMarker, 2) == 2;

    const bool seekSucceeded =
        file.seek(expectedSize - 2, SeekSet);

    const bool endRead =
        file.read(endMarker, 2) == 2;

    file.close();

    return (
        startRead &&
        seekSucceeded &&
        endRead &&
        startMarker[0] == 0xFF &&
        startMarker[1] == 0xD8 &&
        endMarker[0] == 0xFF &&
        endMarker[1] == 0xD9
    );
}

// =========================================================
// Capture and save
// =========================================================

bool captureAndSave(
    size_t imageNumber,
    size_t& imageSize,
    uint32_t& captureTime,
    uint32_t& writeTime
)
{
    char path[64];

    snprintf(
        path,
        sizeof(path),
        "/captures/phase6_%02u.jpg",
        static_cast<unsigned int>(imageNumber)
    );

    if (SD_MMC.exists(path)) {
        if (!SD_MMC.remove(path)) {
            Serial.printf(
                "Could not replace %s\n",
                path
            );

            return false;
        }
    }

    showCaptureStatus(
        imageNumber,
        CAPTURE_COUNT
    );

    const uint32_t captureStart = millis();

    camera_fb_t* frame = esp_camera_fb_get();

    captureTime = millis() - captureStart;

    if (!isValidJpeg(frame)) {
        if (frame != nullptr) {
            esp_camera_fb_return(frame);
        }

        return false;
    }

    imageSize = frame->len;

    const uint32_t writeStart = millis();

    File file = SD_MMC.open(path, FILE_WRITE);

    if (!file) {
        esp_camera_fb_return(frame);
        return false;
    }

    const size_t bytesWritten =
        file.write(frame->buf, frame->len);

    file.flush();
    file.close();

    writeTime = millis() - writeStart;

    esp_camera_fb_return(frame);

    if (bytesWritten != imageSize) {
        return false;
    }

    if (!verifySavedImage(path, imageSize)) {
        return false;
    }

    showImageSaved(
        imageNumber,
        imageSize
    );

    Serial.printf(
        "Image %u saved: %s | bytes=%u | "
        "capture=%lu ms | write=%lu ms | VERIFY=PASS\n",
        static_cast<unsigned int>(imageNumber),
        path,
        static_cast<unsigned int>(imageSize),
        static_cast<unsigned long>(captureTime),
        static_cast<unsigned long>(writeTime)
    );

    delay(900);

    return true;
}

// =========================================================
// Arduino setup
// =========================================================

void setup()
{
    Serial.begin(115200);

    delay(2500);

    Serial.println();
    Serial.println("========================================");
    Serial.println(" ALQAI EmotionCam - Full HW Integration");
    Serial.println("========================================");

    // -----------------------------------------------------
    // OLED
    // -----------------------------------------------------

    if (!initializeOled()) {
        Serial.println("OLED initialization failed.");

        while (true) {
            delay(1000);
        }
    }

    showBootScreen();

    Serial.println("OLED initialize : PASS");

    delay(1000);

    showComponentStatus("OLED", "PASS");

    delay(1000);

    // -----------------------------------------------------
    // Camera
    // -----------------------------------------------------

    showComponentStatus("CAMERA", "INITIALIZING");

    if (!initializeCamera()) {
        showFatalError(
            "CAMERA",
            "Initialization failed."
        );
    }

    if (!warmUpCamera()) {
        showFatalError(
            "CAMERA",
            "Warm-up failed."
        );
    }

    Serial.println("Camera initialize: PASS");

    showComponentStatus("CAMERA", "PASS");

    delay(1000);

    // -----------------------------------------------------
    // SD card
    // -----------------------------------------------------

    showComponentStatus("SD CARD", "MOUNTING");

    if (!initializeSdCard()) {
        showFatalError(
            "SD CARD",
            "Mount failed."
        );
    }

    Serial.println("SD mount         : PASS");

    showComponentStatus("SD CARD", "PASS");

    delay(1000);

    Serial.println("----------------------------------------");
    Serial.printf(
        "Free heap before capture : %u bytes\n",
        ESP.getFreeHeap()
    );

    Serial.printf(
        "Free PSRAM before capture: %u bytes\n",
        ESP.getFreePsram()
    );

    // -----------------------------------------------------
    // Capture three images
    // -----------------------------------------------------

    for (
        size_t imageNumber = 1;
        imageNumber <= CAPTURE_COUNT;
        ++imageNumber
    ) {
        size_t imageSize = 0;
        uint32_t captureTime = 0;
        uint32_t writeTime = 0;

        const bool saved = captureAndSave(
            imageNumber,
            imageSize,
            captureTime,
            writeTime
        );

        if (!saved) {
            showFatalError(
                "CAPTURE",
                "Image save failed."
            );
        }
    }

    Serial.println("----------------------------------------");

    Serial.printf(
        "Free heap after capture  : %u bytes\n",
        ESP.getFreeHeap()
    );

    Serial.printf(
        "Free PSRAM after capture : %u bytes\n",
        ESP.getFreePsram()
    );

    Serial.println("----------------------------------------");
    Serial.println("PHASE 6 RESULT   : ALL TESTS PASSED");
    Serial.println("OLED + Camera + SD are working together.");
    Serial.println("========================================");

    drawReadyFace();
}

// =========================================================
// Stable heartbeat
// =========================================================

void loop()
{
    const uint32_t currentTime = millis();

    if (currentTime - lastHeartbeatTime >= 5000) {
        lastHeartbeatTime = currentTime;

        Serial.printf(
            "[Phase 6 heartbeat] uptime=%lu s | "
            "heap=%u | PSRAM=%u\n",
            currentTime / 1000,
            ESP.getFreeHeap(),
            ESP.getFreePsram()
        );
    }

    delay(10);
}