#include "SdCardModule.h"

#include "AppConfig.h"
#include "FS.h"
#include "SD_MMC.h"

namespace
{
    bool sdMounted = false;
    uint32_t saveCounter = 1;

    bool isValidJpeg(
        const uint8_t* imageData,
        size_t imageLength
    )
    {
        if (
            imageData == nullptr ||
            imageLength < 4
        ) {
            return false;
        }

        return (
            imageData[0] == 0xFF &&
            imageData[1] == 0xD8 &&
            imageData[imageLength - 2] == 0xFF &&
            imageData[imageLength - 1] == 0xD9
        );
    }

    bool verifySavedJpeg(
        const char* path,
        size_t expectedLength
    )
    {
        File file = SD_MMC.open(path, FILE_READ);

        if (!file) {
            return false;
        }

        if (
            file.size() != expectedLength ||
            expectedLength < 4
        ) {
            file.close();
            return false;
        }

        uint8_t firstBytes[2] = {};
        uint8_t lastBytes[2] = {};

        const bool firstRead =
            file.read(firstBytes, 2) == 2;

        const bool seekSucceeded =
            file.seek(expectedLength - 2, SeekSet);

        const bool lastRead =
            file.read(lastBytes, 2) == 2;

        file.close();

        return (
            firstRead &&
            seekSucceeded &&
            lastRead &&
            firstBytes[0] == 0xFF &&
            firstBytes[1] == 0xD8 &&
            lastBytes[0] == 0xFF &&
            lastBytes[1] == 0xD9
        );
    }

    void findNextAvailableFilename()
    {
        char path[64];

        while (saveCounter < 10000) {
            snprintf(
                path,
                sizeof(path),
                "%s/web_%04lu.jpg",
                AppConfig::SdCard::CAPTURE_DIRECTORY,
                static_cast<unsigned long>(saveCounter)
            );

            if (!SD_MMC.exists(path)) {
                return;
            }

            saveCounter++;
        }
    }
}

namespace SdCardModule
{
    bool begin()
    {
        sdMounted = false;

        if (
            !SD_MMC.setPins(
                AppConfig::SdCard::CLK,
                AppConfig::SdCard::CMD,
                AppConfig::SdCard::D0
            )
        ) {
            Serial.println("SD: setPins() failed.");
            return false;
        }

        if (
            !SD_MMC.begin(
                AppConfig::SdCard::MOUNT_POINT,
                true,
                false,
                SDMMC_FREQ_DEFAULT,
                5
            )
        ) {
            Serial.println("SD: mount failed.");
            return false;
        }

        if (SD_MMC.cardType() == CARD_NONE) {
            Serial.println("SD: no card detected.");
            return false;
        }

        if (
            !SD_MMC.exists(
                AppConfig::SdCard::CAPTURE_DIRECTORY
            )
        ) {
            if (
                !SD_MMC.mkdir(
                    AppConfig::SdCard::CAPTURE_DIRECTORY
                )
            ) {
                Serial.println(
                    "SD: could not create /captures."
                );

                return false;
            }
        }

        findNextAvailableFilename();

        sdMounted = true;

        Serial.println("SD-card module        : PASS");
        Serial.printf(
            "SD capacity          : %llu MB\n",
            capacityMB()
        );

        return true;
    }

    bool saveJpeg(
        const uint8_t* imageData,
        size_t imageLength,
        String& savedPath
    )
    {
        savedPath = "";

        if (saveCounter >= 10000) {
            return false;
        }

        char path[64];

        snprintf(
            path,
            sizeof(path),
            "%s/web_%04lu.jpg",
            AppConfig::SdCard::CAPTURE_DIRECTORY,
            static_cast<unsigned long>(saveCounter)
        );

        if (!saveJpegToPath(
                imageData,
                imageLength,
                path
            )) {
            return false;
        }

        savedPath = path;
        saveCounter++;

        return true;
    }

    bool saveJpegToPath(
        const uint8_t* imageData,
        size_t imageLength,
        const char* path
    )
    {
        if (
            !sdMounted ||
            path == nullptr ||
            path[0] == '\0' ||
            !isValidJpeg(imageData, imageLength) ||
            SD_MMC.exists(path)
        ) {
            return false;
        }

        File file = SD_MMC.open(path, FILE_WRITE);

        if (!file) {
            return false;
        }

        const size_t bytesWritten =
            file.write(imageData, imageLength);

        file.flush();
        file.close();

        if (
            bytesWritten != imageLength ||
            !verifySavedJpeg(path, imageLength)
        ) {
            SD_MMC.remove(path);
            return false;
        }

        return true;
    }


bool exists(const char* path)
{
    return (
        sdMounted &&
        path != nullptr &&
        path[0] != '\0' &&
        SD_MMC.exists(path)
    );
}

size_t fileSize(const char* path)
{
    if (!exists(path)) {
        return 0;
    }

    File file = SD_MMC.open(path, FILE_READ);

    if (!file) {
        return 0;
    }

    const size_t size = file.size();
    file.close();

    return size;
}

bool readFile(
    const char* path,
    uint8_t* destination,
    size_t expectedLength
)
{
    if (
        !exists(path) ||
        destination == nullptr ||
        expectedLength == 0
    ) {
        return false;
    }

    File file = SD_MMC.open(path, FILE_READ);

    if (!file || file.size() != expectedLength) {
        if (file) {
            file.close();
        }

        return false;
    }

    size_t totalRead = 0;

    while (totalRead < expectedLength) {
        const size_t bytesRead = file.read(
            destination + totalRead,
            expectedLength - totalRead
        );

        if (bytesRead == 0) {
            file.close();
            return false;
        }

        totalRead += bytesRead;
    }

    file.close();
    return totalRead == expectedLength;
}

    bool isMounted()
    {
        return sdMounted;
    }

    uint64_t capacityMB()
    {
        if (!sdMounted) {
            return 0;
        }

        return (
            SD_MMC.cardSize() /
            (1024ULL * 1024ULL)
        );
    }
}
