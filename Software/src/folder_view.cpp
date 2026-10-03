#include "folder_view.h"

#include <Arduino.h>

#include "audio_player.h"
#include "bluetooth.h"
#include "gui.h"
#include "storage.h"
#include "ui/screens.h"

namespace {
enum FolderEntryKind : uintptr_t {
    FolderEntryDirectory = 1,
    FolderEntryTrack = 2,
    FolderEntryParent = 3
};

String currentDirectory = "/";

void refreshFolderView();

void changeToParentDirectory() {
    if (currentDirectory == "/") {
        return;
    }

    const int lastSlash = currentDirectory.lastIndexOf('/');
    currentDirectory = lastSlash <= 0 ? "/" : currentDirectory.substring(0, lastSlash);
}

void playTrack(const String &path) {
    switch (audioPlayerPlay(path.c_str())) {
        case AudioPlayResult::OpenFailed:
            guiSetStatus("MP3 acilamadi");
            return;
        case AudioPlayResult::NoMemory:
            guiSetStatus("MP3 bellek yetersiz");
            return;
        case AudioPlayResult::Started:
            break;
    }

    bluetoothStart();
    if (bluetoothGetState() != BluetoothState::Connected) {
        guiSetStatus("Ayarlar'dan kulaklık seç");
    }
}

void onFolderEntryClicked(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    const uintptr_t entryKind = reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
    if (entryKind == FolderEntryParent) {
        changeToParentDirectory();
        refreshFolderView();
        return;
    }

    lv_obj_t *button = static_cast<lv_obj_t *>(lv_event_get_target(event));
    lv_obj_t *nameLabel = lv_obj_get_child(button, 1);
    if (nameLabel == nullptr) {
        return;
    }

    const String entryName = lv_label_get_text(nameLabel);
    String entryPath = currentDirectory;
    if (!entryPath.endsWith("/")) {
        entryPath += "/";
    }
    entryPath += entryName;

    if (entryKind == FolderEntryDirectory) {
        currentDirectory = entryPath;
        refreshFolderView();
        return;
    }

    entryPath += ".mp3";
    playTrack(entryPath);
}

void addFolderEntry(const char *name, FolderEntryKind kind, uint16_t rowIndex) {
    lv_obj_t *button = lv_button_create(objects.folder_container);
    lv_obj_align(button, LV_ALIGN_TOP_MID, 0, 4 + rowIndex * 30);
    lv_obj_set_size(button, 224, 28);
    lv_obj_add_event_cb(
        button,
        onFolderEntryClicked,
        LV_EVENT_CLICKED,
        reinterpret_cast<void *>(static_cast<uintptr_t>(kind)));

    lv_obj_t *icon = lv_label_create(button);
    lv_label_set_text(icon, kind == FolderEntryTrack ? LV_SYMBOL_AUDIO : LV_SYMBOL_DIRECTORY);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t *nameLabel = lv_label_create(button);
    lv_label_set_text(nameLabel, name);
    lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_font(nameLabel, &lv_font_turkish_14, LV_PART_MAIN);
    lv_obj_set_width(nameLabel, 188);
    lv_obj_align(nameLabel, LV_ALIGN_LEFT_MID, 24, 0);
}

void addStorageEntry(const char *name, StorageEntryKind kind, void *context) {
    uint16_t &rowIndex = *static_cast<uint16_t *>(context);
    if (kind == StorageEntryKind::Directory) {
        addFolderEntry(name, FolderEntryDirectory, rowIndex++);
        return;
    }

    // Listede ".mp3" uzantisi gosterilmez; tiklaninca geri eklenir.
    String displayName(name);
    displayName.remove(displayName.length() - 4);
    addFolderEntry(displayName.c_str(), FolderEntryTrack, rowIndex++);
}

void refreshFolderView() {
    lv_obj_clean(objects.folder_container);
    lv_obj_set_style_pad_all(objects.folder_container, 0, LV_PART_MAIN);
    const bool folderTabActive = guiActiveTab() == GuiTabFolder;
    if (folderTabActive) {
        guiSetStatus(currentDirectory.c_str());
    }

    uint16_t rowIndex = 0;
    if (storageReady() && currentDirectory != "/") {
        addFolderEntry("geri", FolderEntryParent, rowIndex++);
    }

    const StorageListResult result =
        storageListDirectory(currentDirectory.c_str(), addStorageEntry, &rowIndex);
    if (!folderTabActive) {
        return;
    }

    switch (result) {
        case StorageListResult::NotReady:
            guiSetStatus("SD kart hazir degil");
            break;
        case StorageListResult::OpenFailed:
            guiSetStatus("Klasor acilamadi");
            break;
        case StorageListResult::Ok:
            if (rowIndex == 0) {
                guiSetStatus("Klasor bos veya MP3 yok");
            }
            break;
    }
}
}

void folderViewCreate() {
    refreshFolderView();
}

const char *folderViewCurrentPath() {
    return currentDirectory.c_str();
}
