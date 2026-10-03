#pragma once

#include <SdFat.h>

enum class StorageEntryKind {
    Directory,
    Track
};

enum class StorageListResult {
    Ok,
    NotReady,
    OpenFailed
};

// Klasor listelenirken her alt klasor ve .mp3 dosyasi icin cagrilir; name tam dosya adidir.
using StorageEntryCallback = void (*)(const char *name, StorageEntryKind kind, void *context);

bool storageBegin();
bool storageReady();

// "System Volume Information" atlanir, sadece klasorler ve .mp3 dosyalari bildirilir.
StorageListResult storageListDirectory(const char *path, StorageEntryCallback callback, void *context);

// Dosya islemleri SPI kilidini kendileri alir; baska gorevlerden guvenle cagrilabilir.
bool storageOpenFile(FsFile &file, const char *path);
int storageReadFile(FsFile &file, uint8_t *buffer, size_t size);
