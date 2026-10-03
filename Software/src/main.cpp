#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>
#include "ui/ui.h"
#include "ui/screens.h"

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
constexpr uint32_t drawBufferRows = 20;

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
lv_obj_t *coordinateLabels[3];
lv_color_t drawBuffer[320 * drawBufferRows];

// Ekran ve SD ayni bus'i kullandigi icin erisimleri sirala
SemaphoreHandle_t spiBusMutex;
bool sdReady = false;

uint32_t getMillis() {
    return millis();
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
        for (lv_obj_t *label : coordinateLabels) {
            lv_label_set_text_fmt(label, "Ekran basladi\nX: %d   Y: %d", lastX, lastY);
        }
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->point.x = lastX;
    data->point.y = lastY;
}

void createCoordinateLabels() {
    lv_obj_t *screens[] = { objects.sc_play, objects.sc_folder, objects.sc_settings };
    for (size_t index = 0; index < 3; ++index) {
        coordinateLabels[index] = lv_label_create(screens[index]);
        lv_label_set_text(coordinateLabels[index], "Ekran basladi\nX: --   Y: --");
        lv_obj_align(coordinateLabels[index], LV_ALIGN_TOP_MID, 0, 10);
    }
}

void lvglTask(void *) {
    for (;;) {
        ui_tick();
        const uint32_t waitMs = lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(waitMs < 5 ? 5 : waitMs));
    }
}

// SD'ye her erisimde bunlari kullan
void sdLock()   { xSemaphoreTakeRecursive(spiBusMutex, portMAX_DELAY); }
void sdUnlock() { xSemaphoreGiveRecursive(spiBusMutex); }

void listSdRoot() {
    if (!sdReady) return;
    sdLock();
    FsFile root;
    if (root.open("/", O_RDONLY)) {
        FsFile entry;
        char entryName[256];
        while (entry.openNext(&root, O_RDONLY)) {
            entry.getName(entryName, sizeof(entryName));
            Serial.printf("%s%s  (%llu bayt)\n",
                          entryName,
                          entry.isDir() ? "/" : "",
                          static_cast<unsigned long long>(entry.fileSize()));
            entry.close();
        }
        root.close();
    }
    sdUnlock();
}
}

void setup() {
    Serial.begin(115200);

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

    Serial.printf("SD SoftSPI: CS=%u SCK=%u MISO=%u MOSI=%u\n",
                  sdChipSelectPin,
                  sdClockPin,
                  sdMisoPin,
                  sdMosiPin);
    sdLock();
    sdReady = sdCard.begin(SdSpiConfig(
        sdChipSelectPin,
        DEDICATED_SPI,
        SD_SCK_MHZ(0),
        &sdSoftSpi));
    sdUnlock();
    Serial.println(sdReady ? "SD kart hazir" : "SD kart baslatilamadi");
    if (!sdReady) {
        sdCard.initErrorPrint();
    }
    listSdRoot();

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
    createCoordinateLabels();
    xTaskCreatePinnedToCore(lvglTask, "LVGL", 12 * 1024, nullptr, 2, nullptr, 1);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}