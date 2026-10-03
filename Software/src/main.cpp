#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <lvgl.h>
#include <time.h>
#include "ui/ui.h"
#include "ui/screens.h"

#if __has_include("wifi_passwords.h")
#include "wifi_passwords.h"
#else
#define MBMP3_WIFI_SSID ""
#define MBMP3_WIFI_PASSWORD ""
#endif

extern "C" {
LV_FONT_DECLARE(lv_font_turkish_14);
}

namespace {
constexpr int touchIrqPin = 36;
constexpr int touchMosiPin = 32;
constexpr int touchMisoPin = 39;
constexpr int touchClockPin = 25;
constexpr int touchChipSelectPin = 33;
constexpr int touchXAtLeft = 240;
constexpr int touchXAtRight = 0;
constexpr int touchYAtTop = 320;
constexpr int touchYAtBottom = 0;
constexpr uint32_t drawBufferRows = 12;

// SD kart yazilimsal SPI pinleri; TFT ve dokunmatikten bagimsiz
constexpr int sdChipSelectPin = 26;
constexpr uint8_t sdMisoPin = 19;
constexpr uint8_t sdMosiPin = 23;
constexpr uint8_t sdClockPin = 18;

TFT_eSPI tft;
SPIClass touchscreenSPI(SPI);
SoftSpiDriver<sdMisoPin, sdMosiPin, sdClockPin> sdSoftSpi;
SdFs sdCard;
XPT2046_Touchscreen touchscreen(touchChipSelectPin, touchIrqPin);
lv_display_t *display;
lv_color_t drawBuffer[320 * drawBufferRows];

SemaphoreHandle_t spiBusMutex;
bool sdReady = false;
bool ntpConfigured = false;
uint32_t lastWifiUiUpdate = 0;
String currentDirectory = "/";
String selectedTrackPath;

enum FolderEntryKind : uintptr_t {
    FolderEntryDirectory = 1,
    FolderEntryTrack = 2,
    FolderEntryParent = 3
};

uint32_t getMillis() {
    return millis();
}

bool wifiCredentialsAvailable() {
    return MBMP3_WIFI_SSID[0] != '\0';
}

void startWifiConnection() {
    if (!wifiCredentialsAvailable()) {
        WiFi.mode(WIFI_OFF);
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(MBMP3_WIFI_SSID, MBMP3_WIFI_PASSWORD);
}

void updateClock() {
    if (WiFi.status() == WL_CONNECTED && !ntpConfigured) {
        configTzTime("TRT-3", "pool.ntp.org", "time.cloudflare.com");
        ntpConfigured = true;
    }

    struct tm localTime;
    if (getLocalTime(&localTime, 10)) {
        lv_label_set_text_fmt(objects.time, "%02d:%02d", localTime.tm_hour, localTime.tm_min);
    }
}

void flushDisplay(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
    const uint32_t width = area->x2 - area->x1 + 1;
    const uint32_t height = area->y2 - area->y1 + 1;

    xSemaphoreTakeRecursive(spiBusMutex, portMAX_DELAY);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, width, height);
    tft.pushColors(reinterpret_cast<uint16_t *>(pixels), width * height, true);
    tft.endWrite();
    xSemaphoreGiveRecursive(spiBusMutex);

    lv_display_flush_ready(display);
}

void readTouch(lv_indev_t *, lv_indev_data_t *data) {
    static int16_t lastX = 0;
    static int16_t lastY = 0;

    if (touchscreen.tirqTouched() && touchscreen.touched()) {
        const TS_Point point = touchscreen.getPoint();
        const long measuredX = map(point.x, 200, 3700, 0, tft.width() - 1);
        const long measuredY = map(point.y, 240, 3800, 0, tft.height() - 1);
        lastX = constrain(map(measuredX, touchXAtLeft, touchXAtRight, 0, tft.width() - 1), 0, tft.width() - 1);
        lastY = constrain(map(measuredY, touchYAtTop, touchYAtBottom, 0, tft.height() - 1), 0, tft.height() - 1);
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->point.x = lastX;
    data->point.y = lastY;
}

void lvglTask(void *) {
    for (;;) {
        ui_tick();
        if (millis() - lastWifiUiUpdate >= 1000) {
            lastWifiUiUpdate = millis();
            updateClock();
        }
        const uint32_t waitMs = lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(waitMs < 5 ? 5 : waitMs));
    }
}

void sdLock()   { xSemaphoreTakeRecursive(spiBusMutex, portMAX_DELAY); }
void sdUnlock() { xSemaphoreGiveRecursive(spiBusMutex); }

void refreshFolderView();

void changeToParentDirectory() {
    if (currentDirectory == "/") {
        return;
    }

    const int lastSlash = currentDirectory.lastIndexOf('/');
    currentDirectory = lastSlash <= 0 ? "/" : currentDirectory.substring(0, lastSlash);
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
    selectedTrackPath = entryPath;
    lv_label_set_text_fmt(objects.current_screen, "Secildi: %s", selectedTrackPath.c_str());
}

void addFolderEntry(const String &name, FolderEntryKind kind, uint16_t rowIndex) {
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
    lv_label_set_text(nameLabel, name.c_str());
    lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_font(nameLabel, &lv_font_turkish_14, LV_PART_MAIN);
    lv_obj_set_width(nameLabel, 188);
    lv_obj_align(nameLabel, LV_ALIGN_LEFT_MID, 24, 0);
}

void refreshFolderView() {
    lv_obj_clean(objects.folder_container);
    lv_obj_set_style_pad_all(objects.folder_container, 0, LV_PART_MAIN);
    const bool folderTabActive = lv_tabview_get_tab_active(objects.tab_view) == 1;
    if (folderTabActive) {
        lv_label_set_text(objects.current_screen, currentDirectory.c_str());
    }

    if (!sdReady) {
        if (folderTabActive) {
            lv_label_set_text(objects.current_screen, "SD kart hazir degil");
        }
        return;
    }

    sdLock();
    FsFile directory;
    if (!directory.open(currentDirectory.c_str(), O_RDONLY) || !directory.isDir()) {
        sdUnlock();
        if (folderTabActive) {
            lv_label_set_text(objects.current_screen, "Klasor acilamadi");
        }
        if (directory.isOpen()) {
            directory.close();
        }
        return;
    }

    uint16_t rowIndex = 0;
    if (currentDirectory != "/") {
        addFolderEntry("geri", FolderEntryParent, rowIndex++);
    }

    FsFile entry;
    char entryName[256];
    while (entry.openNext(&directory, O_RDONLY)) {
        entry.getName(entryName, sizeof(entryName));
        String name(entryName);
        if (entry.isDir()) {
            String lowercaseName = name;
            lowercaseName.toLowerCase();
            if (lowercaseName == "system volume information") {
                entry.close();
                continue;
            }
            addFolderEntry(name, FolderEntryDirectory, rowIndex++);
        } else {
            String lowercaseName = name;
            lowercaseName.toLowerCase();
            if (lowercaseName.endsWith(".mp3")) {
                name.remove(name.length() - 4);
                addFolderEntry(name, FolderEntryTrack, rowIndex++);
            }
        }
        entry.close();
    }
    directory.close();
    sdUnlock();

    if (rowIndex == 0 && folderTabActive) {
        lv_label_set_text(objects.current_screen, "Klasor bos veya MP3 yok");
    }
}

void onTabChanged(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    switch (lv_tabview_get_tab_active(objects.tab_view)) {
        case 0:
            lv_label_set_text(objects.current_screen, "Play");
            break;
        case 1:
            lv_label_set_text(objects.current_screen, currentDirectory.c_str());
            break;
        case 2:
            lv_label_set_text(objects.current_screen, "Settings");
            break;
    }
}
}

void setup() {
    spiBusMutex = xSemaphoreCreateRecursiveMutex();

    // SD CS'i SD baslatilmadan once pasif tut.
    pinMode(sdChipSelectPin, OUTPUT);
    digitalWrite(sdChipSelectPin, HIGH);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    touchscreenSPI.begin(touchClockPin, touchMisoPin, touchMosiPin, touchChipSelectPin);
    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(0);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    sdLock();
    sdReady = sdCard.begin(SdSpiConfig(
        sdChipSelectPin,
        DEDICATED_SPI,
        SD_SCK_MHZ(0),
        &sdSoftSpi));
    sdUnlock();

    lv_init();
    lv_tick_set_cb(getMillis);

    display = lv_display_create(tft.width(), tft.height());
    lv_display_set_flush_cb(display, flushDisplay);
    lv_display_set_buffers(
        display,
        drawBuffer,
        nullptr,
        sizeof(drawBuffer),
        LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *touchInput = lv_indev_create();
    lv_indev_set_type(touchInput, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(touchInput, display);
    lv_indev_set_read_cb(touchInput, readTouch);

    ui_init();
    lv_label_set_text(objects.time, "--:--");
    startWifiConnection();
    lv_label_set_text(objects.current_screen, "Play");
    lv_obj_set_pos(objects.current_screen, 52, 12);
    lv_obj_set_size(objects.current_screen, 136, 18);
    lv_label_set_long_mode(objects.current_screen, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_align(objects.current_screen, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(objects.current_screen, &lv_font_turkish_14, LV_PART_MAIN);
    lv_obj_add_event_cb(objects.tab_view, onTabChanged, LV_EVENT_VALUE_CHANGED, nullptr);
    refreshFolderView();
    xTaskCreatePinnedToCore(lvglTask, "LVGL", 12 * 1024, nullptr, 2, nullptr, 1);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}