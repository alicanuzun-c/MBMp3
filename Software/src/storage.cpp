#include "storage.h"

#include <Arduino.h>

#include "board_config.h"

namespace {
SoftSpiDriver<sdMisoPin, sdMosiPin, sdClockPin> sdSoftSpi;
SdFs sdCard;
bool sdReady = false;
// SD yazilimsal SPI ile kendi pinlerinde; kilit sadece SdFat'a ayni anda iki gorevin
// (arayuz ve MP3 cozucu) erismesini engeller, ekran aktarimini beklemez.
SemaphoreHandle_t storageMutex;

void storageLock() {
    xSemaphoreTake(storageMutex, portMAX_DELAY);
}

void storageUnlock() {
    xSemaphoreGive(storageMutex);
}

bool endsWithMp3(const char *name) {
    const size_t length = strlen(name);
    return length >= 4 && strcasecmp(name + length - 4, ".mp3") == 0;
}
}

bool storageBegin() {
    // SD CS'i SD baslatilmadan once pasif tut.
    pinMode(sdChipSelectPin, OUTPUT);
    digitalWrite(sdChipSelectPin, HIGH);

    storageMutex = xSemaphoreCreateMutex();
    storageLock();
    sdReady = sdCard.begin(SdSpiConfig(
        sdChipSelectPin,
        DEDICATED_SPI,
        SD_SCK_MHZ(0),
        &sdSoftSpi));
    storageUnlock();
    return sdReady;
}

bool storageReady() {
    return sdReady;
}

StorageListResult storageListDirectory(const char *path, StorageEntryCallback callback, void *context) {
    if (!sdReady) {
        return StorageListResult::NotReady;
    }

    storageLock();
    FsFile directory;
    if (!directory.open(path, O_RDONLY) || !directory.isDir()) {
        if (directory.isOpen()) {
            directory.close();
        }
        storageUnlock();
        return StorageListResult::OpenFailed;
    }

    FsFile entry;
    char entryName[256];
    while (entry.openNext(&directory, O_RDONLY)) {
        entry.getName(entryName, sizeof(entryName));
        const bool isDirectory = entry.isDir();
        entry.close();

        // Arayuz satiri olusturulurken kilidi birak; MP3 cozucu okumaya devam edebilsin.
        storageUnlock();
        if (isDirectory) {
            if (strcasecmp(entryName, "System Volume Information") != 0) {
                callback(entryName, StorageEntryKind::Directory, context);
            }
        } else if (endsWithMp3(entryName)) {
            callback(entryName, StorageEntryKind::Track, context);
        }
        storageLock();
    }
    directory.close();
    storageUnlock();
    return StorageListResult::Ok;
}

bool storageOpenFile(FsFile &file, const char *path) {
    storageLock();
    if (file.isOpen()) {
        file.close();
    }
    const bool opened = file.open(path, O_RDONLY);
    storageUnlock();
    return opened;
}

int storageReadFile(FsFile &file, uint8_t *buffer, size_t size) {
    storageLock();
    const int bytesRead = file.read(buffer, size);
    storageUnlock();
    return bytesRead;
}
