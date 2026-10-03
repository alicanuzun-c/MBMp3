#include "storage.h"

#include <Arduino.h>

#include "board_config.h"
#include "spi_bus.h"

namespace {
SoftSpiDriver<sdMisoPin, sdMosiPin, sdClockPin> sdSoftSpi;
SdFs sdCard;
bool sdReady = false;

bool endsWithMp3(const char *name) {
    const size_t length = strlen(name);
    return length >= 4 && strcasecmp(name + length - 4, ".mp3") == 0;
}
}

bool storageBegin() {
    // SD CS'i SD baslatilmadan once pasif tut.
    pinMode(sdChipSelectPin, OUTPUT);
    digitalWrite(sdChipSelectPin, HIGH);

    spiBusLock();
    sdReady = sdCard.begin(SdSpiConfig(
        sdChipSelectPin,
        DEDICATED_SPI,
        SD_SCK_MHZ(0),
        &sdSoftSpi));
    spiBusUnlock();
    return sdReady;
}

bool storageReady() {
    return sdReady;
}

StorageListResult storageListDirectory(const char *path, StorageEntryCallback callback, void *context) {
    if (!sdReady) {
        return StorageListResult::NotReady;
    }

    spiBusLock();
    FsFile directory;
    if (!directory.open(path, O_RDONLY) || !directory.isDir()) {
        if (directory.isOpen()) {
            directory.close();
        }
        spiBusUnlock();
        return StorageListResult::OpenFailed;
    }

    FsFile entry;
    char entryName[256];
    while (entry.openNext(&directory, O_RDONLY)) {
        entry.getName(entryName, sizeof(entryName));
        if (entry.isDir()) {
            if (strcasecmp(entryName, "System Volume Information") != 0) {
                callback(entryName, StorageEntryKind::Directory, context);
            }
        } else if (endsWithMp3(entryName)) {
            callback(entryName, StorageEntryKind::Track, context);
        }
        entry.close();
    }
    directory.close();
    spiBusUnlock();
    return StorageListResult::Ok;
}

bool storageOpenFile(FsFile &file, const char *path) {
    spiBusLock();
    if (file.isOpen()) {
        file.close();
    }
    const bool opened = file.open(path, O_RDONLY);
    spiBusUnlock();
    return opened;
}

int storageReadFile(FsFile &file, uint8_t *buffer, size_t size) {
    spiBusLock();
    const int bytesRead = file.read(buffer, size);
    spiBusUnlock();
    return bytesRead;
}
