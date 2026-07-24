#include "OledModule.h"

#include <cstring>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "AppConfig.h"

namespace
{
    Adafruit_SSD1306 display(
        AppConfig::Oled::WIDTH,
        AppConfig::Oled::HEIGHT,
        &Wire,
        AppConfig::Oled::RESET
    );

    enum class EmotionStyle : uint8_t
    {
        Happy,
        Neutral,
        Sad,
        Surprise,
        Sleepy,
        Wink,
        Excited
    };

    EmotionStyle emotionStyleFromLabel(const char* label)
    {
        if (label == nullptr) return EmotionStyle::Neutral;
        if (strcasecmp(label, "happy") == 0) return EmotionStyle::Happy;
        if (strcasecmp(label, "sad") == 0) return EmotionStyle::Sad;
        if (strcasecmp(label, "surprise") == 0) return EmotionStyle::Surprise;
        if (strcasecmp(label, "neutral") == 0) return EmotionStyle::Neutral;
        return EmotionStyle::Neutral;
    }

    bool oledReady = false;

    bool emotionTransitionPending = false;
    uint32_t emotionTransitionAt = 0;

    char pendingEmotionLabel[20] = "LIVE READY";
    EmotionStyle pendingEmotionStyle = EmotionStyle::Happy;

    void clearDisplay()
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
        int16_t x1 = 0;
        int16_t y1 = 0;
        uint16_t width = 0;
        uint16_t height = 0;

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

        display.setCursor(
            (AppConfig::Oled::WIDTH - width) / 2,
            y
        );

        display.print(text);
    }

    void drawSmallEmotionFace(EmotionStyle style)
    {
        constexpr int faceX = 36;
        constexpr int faceY = 14;
        constexpr int faceWidth = 56;
        constexpr int faceHeight = 36;

        display.drawRoundRect(
            faceX,
            faceY,
            faceWidth,
            faceHeight,
            10,
            SSD1306_WHITE
        );

        switch (style) {
            case EmotionStyle::Neutral:
                // Eyes & pupils
                display.fillCircle(50, 26, 3, SSD1306_WHITE);
                display.fillCircle(78, 26, 3, SSD1306_WHITE);
                // Calm eyebrows
                display.drawLine(46, 20, 54, 20, SSD1306_WHITE);
                display.drawLine(74, 20, 82, 20, SSD1306_WHITE);
                // Straight mouth
                display.drawLine(58, 38, 70, 38, SSD1306_WHITE);
                break;

            case EmotionStyle::Happy:
                // Curved happy eyes (^ ^)
                display.drawLine(46, 27, 50, 23, SSD1306_WHITE);
                display.drawLine(50, 23, 54, 27, SSD1306_WHITE);
                display.drawLine(74, 27, 78, 23, SSD1306_WHITE);
                display.drawLine(78, 23, 82, 27, SSD1306_WHITE);
                // Blushing cheeks
                display.fillCircle(44, 32, 2, SSD1306_WHITE);
                display.fillCircle(84, 32, 2, SSD1306_WHITE);
                // Wide smile curve
                display.drawLine(54, 36, 58, 41, SSD1306_WHITE);
                display.drawLine(58, 41, 70, 41, SSD1306_WHITE);
                display.drawLine(70, 41, 74, 36, SSD1306_WHITE);
                break;

            case EmotionStyle::Sad:
                // Drooping pupils
                display.fillCircle(50, 27, 3, SSD1306_WHITE);
                display.fillCircle(78, 27, 3, SSD1306_WHITE);
                // Slanted sad eyebrows (\ /)
                display.drawLine(46, 20, 54, 24, SSD1306_WHITE);
                display.drawLine(82, 20, 74, 24, SSD1306_WHITE);
                // Downturned frown arc
                display.drawLine(54, 41, 58, 37, SSD1306_WHITE);
                display.drawLine(58, 37, 70, 37, SSD1306_WHITE);
                display.drawLine(70, 37, 74, 41, SSD1306_WHITE);
                // Tear drop
                display.fillCircle(45, 34, 1, SSD1306_WHITE);
                display.drawLine(45, 32, 45, 34, SSD1306_WHITE);
                break;

            case EmotionStyle::Surprise:
                // High raised eyebrows
                display.drawLine(46, 18, 50, 16, SSD1306_WHITE);
                display.drawLine(50, 16, 54, 18, SSD1306_WHITE);
                display.drawLine(74, 18, 78, 16, SSD1306_WHITE);
                display.drawLine(78, 16, 82, 18, SSD1306_WHITE);
                // Wide open concentric eyes
                display.drawCircle(50, 25, 4, SSD1306_WHITE);
                display.fillCircle(50, 25, 1, SSD1306_WHITE);
                display.drawCircle(78, 25, 4, SSD1306_WHITE);
                display.fillCircle(78, 25, 1, SSD1306_WHITE);
                // Big open surprise "O" mouth
                display.drawRoundRect(59, 33, 10, 11, 4, SSD1306_WHITE);
                break;

            case EmotionStyle::Sleepy:
                display.drawLine(46, 26, 54, 26, SSD1306_WHITE);
                display.drawLine(74, 26, 82, 26, SSD1306_WHITE);
                display.drawLine(58, 39, 70, 39, SSD1306_WHITE);
                break;

            case EmotionStyle::Wink:
                display.drawLine(46, 26, 54, 26, SSD1306_WHITE);
                display.fillCircle(78, 26, 3, SSD1306_WHITE);
                display.drawLine(54, 36, 58, 40, SSD1306_WHITE);
                display.drawLine(58, 40, 70, 40, SSD1306_WHITE);
                display.drawLine(70, 40, 74, 36, SSD1306_WHITE);
                break;

            case EmotionStyle::Excited:
                display.drawCircle(50, 25, 4, SSD1306_WHITE);
                display.drawCircle(78, 25, 4, SSD1306_WHITE);
                display.fillCircle(50, 25, 1, SSD1306_WHITE);
                display.fillCircle(78, 25, 1, SSD1306_WHITE);
                display.drawCircle(64, 37, 4, SSD1306_WHITE);
                break;
        }
    }

    void showEmotionScreen(
        const char* label,
        EmotionStyle style
    )
    {
        if (!oledReady) {
            return;
        }

        clearDisplay();

        drawCenteredText("ALQAI EMOTIONCAM", 2, 1);
        drawSmallEmotionFace(style);
        drawCenteredText(label, 53, 1);

        display.display();
    }

    void scheduleEmotionScreen(
        const char* label,
        EmotionStyle style,
        uint32_t delayMs
    )
    {
        strncpy(
            pendingEmotionLabel,
            label,
            sizeof(pendingEmotionLabel) - 1
        );

        pendingEmotionLabel[
            sizeof(pendingEmotionLabel) - 1
        ] = '\0';

        pendingEmotionStyle = style;
        emotionTransitionAt = millis() + delayMs;
        emotionTransitionPending = true;
    }

    void showActionScreen(
        const char* title,
        const char* subtitle
    )
    {
        if (!oledReady) {
            return;
        }

        clearDisplay();

        drawCenteredText("ALQAI", 3, 1);
        drawCenteredText(title, 22, 2);
        drawCenteredText(subtitle, 51, 1);

        display.display();
    }
}

namespace OledModule
{
    bool begin()
    {
        if (
            !Wire.begin(
                AppConfig::Oled::SDA,
                AppConfig::Oled::SCL,
                AppConfig::Oled::I2C_FREQUENCY_HZ
            )
        ) {
            Serial.println("OLED: Wire.begin() failed.");
            return false;
        }

        Wire.beginTransmission(
            AppConfig::Oled::ADDRESS
        );

        if (Wire.endTransmission() != 0) {
            Serial.println(
                "OLED: no response at address 0x3C."
            );

            return false;
        }

        if (
            !display.begin(
                SSD1306_SWITCHCAPVCC,
                AppConfig::Oled::ADDRESS,
                true,
                false
            )
        ) {
            Serial.println(
                "OLED: SSD1306 initialization failed."
            );

            return false;
        }

        oledReady = true;
        display.dim(false);
        display.setTextWrap(false);

        Serial.println("OLED module           : PASS");
        return true;
    }

    void showBoot()
    {
        if (!oledReady) {
            return;
        }

        emotionTransitionPending = false;
        clearDisplay();

        drawCenteredText("ALQAI", 7, 2);
        drawCenteredText("EmotionCam", 34, 1);
        drawCenteredText("STARTING...", 51, 1);

        display.display();
    }

    void showNetworkReady(const IPAddress& ipAddress)
    {
        if (!oledReady) {
            return;
        }

        // This screen intentionally has no timer. It remains until
        // handleRoot() confirms that the website was opened.
        emotionTransitionPending = false;
        clearDisplay();

        drawCenteredText("WI-FI READY", 2, 1);

        display.setTextSize(1);

        display.setCursor(2, 18);
        display.print("SSID:");

        display.setCursor(2, 29);
        display.print(AppConfig::Wifi::AP_SSID);

        display.setCursor(2, 45);
        display.print("IP:");

        display.setCursor(20, 45);
        display.print(ipAddress);

        display.display();
    }

    void showWebsiteConnected()
    {
        showActionScreen("WEB", "CONNECTED");

        scheduleEmotionScreen(
            "LIVE READY",
            EmotionStyle::Happy,
            1100
        );
    }

    void showLive()
    {
        showActionScreen("LIVE", "STREAM RESUMED");

        scheduleEmotionScreen(
            "LIVE STREAM",
            EmotionStyle::Happy,
            850
        );
    }

    void showPaused()
    {
        showActionScreen("PAUSED", "PREVIEW STOPPED");

        scheduleEmotionScreen(
            "PREVIEW PAUSED",
            EmotionStyle::Sleepy,
            850
        );
    }

    void showCapturing()
    {
        emotionTransitionPending = false;
        showActionScreen("CAPTURE", "PLEASE WAIT");
    }

    void showCaptured()
    {
        showActionScreen("CAPTURED", "SNAPSHOT READY");

        scheduleEmotionScreen(
            "SNAPSHOT READY",
            EmotionStyle::Wink,
            850
        );
    }

    void showSaving()
    {
        emotionTransitionPending = false;
        showActionScreen("SAVING", "WRITING TO SD");
    }

    void showSaved(
        const char* filename,
        size_t imageBytes
    )
    {
        if (!oledReady) {
            return;
        }

        char bytesText[24];

        snprintf(
            bytesText,
            sizeof(bytesText),
            "%u BYTES",
            static_cast<unsigned int>(imageBytes)
        );

        clearDisplay();

        drawCenteredText("IMAGE SAVED", 3, 1);
        drawCenteredText(filename, 24, 1);
        drawCenteredText(bytesText, 42, 1);
        drawCenteredText("SD: PASS", 54, 1);

        display.display();

        scheduleEmotionScreen(
            "IMAGE SAVED",
            EmotionStyle::Excited,
            1300
        );
    }


void showAiLoading()
{
    emotionTransitionPending = false;
    showActionScreen("AI MODEL", "LOADING FROM SD");
}

void showAiReady(
    size_t modelBytes,
    size_t arenaUsedBytes
)
{
    if (!oledReady) {
        return;
    }

    char modelText[24];
    char arenaText[24];

    snprintf(
        modelText,
        sizeof(modelText),
        "MODEL %u KB",
        static_cast<unsigned int>(
            modelBytes / 1024U
        )
    );

    snprintf(
        arenaText,
        sizeof(arenaText),
        "ARENA %u KB",
        static_cast<unsigned int>(
            arenaUsedBytes / 1024U
        )
    );

    emotionTransitionPending = false;
    clearDisplay();

    drawCenteredText("AI MODEL PASS", 3, 1);
    drawCenteredText(modelText, 23, 1);
    drawCenteredText(arenaText, 38, 1);
    drawCenteredText("INT8 READY", 53, 1);

    display.display();
}

void showAiError(const char* reason)
{
    if (!oledReady) {
        return;
    }

    emotionTransitionPending = false;
    clearDisplay();

    drawCenteredText("AI MODEL ERROR", 4, 1);
    drawCenteredText(reason, 27, 1);
    drawCenteredText("CHECK SERIAL", 51, 1);

    display.display();
}

void showAiCapture()
{
    if (!oledReady) return;
    emotionTransitionPending = false;
    showActionScreen("AI CAPTURE", "PLEASE WAIT");
}

void showAiProcessing(const char* detail)
{
    if (!oledReady) return;
    emotionTransitionPending = false;
    showActionScreen("AI PROCESS", detail != nullptr ? detail : "DECODING...");
}

void showAiInference()
{
    if (!oledReady) return;
    emotionTransitionPending = false;
    showActionScreen("AI INFER", "RUNNING...");
}

void showAiResult(const char* label, uint32_t totalMs)
{
    if (!oledReady) return;

    char timeText[24];
    snprintf(timeText, sizeof(timeText), "%.2f SEC", totalMs / 1000.0f);

    clearDisplay();
    drawCenteredText("AI RESULT", 3, 1);
    drawCenteredText(label, 23, 2);
    drawCenteredText(timeText, 51, 1);
    display.display();

    scheduleEmotionScreen(
        label,
        emotionStyleFromLabel(label),
        4500
    );
}

    void showDatasetCapturing(const char* label)
    {
        if (!oledReady) {
            return;
        }

        emotionTransitionPending = false;
        clearDisplay();

        drawCenteredText("DATASET", 2, 1);
        drawCenteredText(label, 20, 2);
        drawCenteredText("CAPTURING...", 51, 1);

        display.display();
    }

    void showDatasetSaved(
        const char* label,
        uint32_t imageIndex
    )
    {
        if (!oledReady) {
            return;
        }

        char imageText[24];

        snprintf(
            imageText,
            sizeof(imageText),
            "IMAGE %04lu",
            static_cast<unsigned long>(imageIndex)
        );

        clearDisplay();

        drawCenteredText(label, 3, 1);
        drawCenteredText("SAVED", 19, 2);
        drawCenteredText(imageText, 49, 1);

        display.display();

        scheduleEmotionScreen(
            "DATASET SAVED",
            EmotionStyle::Excited,
            1300
        );
    }

    void showDatasetError(const char* reason)
    {
        if (!oledReady) {
            return;
        }

        emotionTransitionPending = false;
        clearDisplay();

        drawCenteredText("DATASET ERROR", 5, 1);
        drawCenteredText(reason, 26, 1);
        drawCenteredText("CHECK SERIAL", 50, 1);

        display.display();
    }

    void showActionError(const char* action)
    {
        if (!oledReady) {
            return;
        }

        emotionTransitionPending = false;
        clearDisplay();

        drawCenteredText("ACTION ERROR", 5, 1);
        drawCenteredText(action, 27, 2);
        drawCenteredText("CHECK SERIAL", 52, 1);

        display.display();
    }

    void showFatal(const char* component)
    {
        if (!oledReady) {
            return;
        }

        emotionTransitionPending = false;
        clearDisplay();

        drawCenteredText("SYSTEM ERROR", 5, 1);
        drawCenteredText(component, 27, 2);
        drawCenteredText("CHECK SERIAL", 52, 1);

        display.display();
    }

    void update()
    {
        if (
            !oledReady ||
            !emotionTransitionPending
        ) {
            return;
        }

        if (
            static_cast<int32_t>(
                millis() - emotionTransitionAt
            ) < 0
        ) {
            return;
        }

        emotionTransitionPending = false;

        showEmotionScreen(
            pendingEmotionLabel,
            pendingEmotionStyle
        );
    }
}
