#include "gui.h"

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "board_config.h"
#include "folder_view.h"
#include "settings_view.h"
#include "spi_bus.h"
#include "ui/screens.h"
#include "ui/ui.h"
#include "wifi_clock.h"

namespace {
constexpr int touchXAtLeft = 240;
constexpr int touchXAtRight = 0;
constexpr int touchYAtTop = 320;
constexpr int touchYAtBottom = 0;
constexpr uint32_t drawBufferRows = 8;
constexpr size_t drawBufferSize = 320 * drawBufferRows * sizeof(lv_color_t);

TFT_eSPI tft;
SPIClass touchscreenSPI(SPI);
XPT2046_Touchscreen touchscreen(touchChipSelectPin, touchIrqPin);
lv_display_t *display;
lv_color_t *drawBuffer;
uint32_t lastSecondTick = 0;

uint32_t getMillis() {
    return millis();
}

void flushDisplay(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
    const uint32_t width = area->x2 - area->x1 + 1;
    const uint32_t height = area->y2 - area->y1 + 1;

    spiBusLock();
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, width, height);
    tft.pushColors(reinterpret_cast<uint16_t *>(pixels), width * height, true);
    tft.endWrite();
    spiBusUnlock();

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

void updateClock() {
    struct tm localTime;
    if (wifiClockLocalTime(localTime)) {
        lv_label_set_text_fmt(objects.time, "%02d:%02d", localTime.tm_hour, localTime.tm_min);
    }
}

void lvglTask(void *) {
    for (;;) {
        ui_tick();
        if (millis() - lastSecondTick >= 1000) {
            lastSecondTick = millis();
            updateClock();
            if (guiActiveTab() == GuiTabSettings) {
                settingsViewRefresh();
            }
        }
        const uint32_t waitMs = lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(waitMs < 5 ? 5 : waitMs));
    }
}

void onTabChanged(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    switch (guiActiveTab()) {
        case GuiTabPlay:
            guiSetStatus("Play");
            break;
        case GuiTabFolder:
            guiSetStatus(folderViewCurrentPath());
            break;
        case GuiTabSettings:
            guiSetStatus("Settings");
            break;
    }
}
}

void guiBegin() {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    touchscreenSPI.begin(touchClockPin, touchMisoPin, touchMosiPin, touchChipSelectPin);
    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(0);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    lv_init();
    lv_tick_set_cb(getMillis);

    drawBuffer = static_cast<lv_color_t *>(heap_caps_malloc(drawBufferSize, MALLOC_CAP_DMA));

    display = lv_display_create(tft.width(), tft.height());
    lv_display_set_flush_cb(display, flushDisplay);
    lv_display_set_buffers(
        display,
        drawBuffer,
        nullptr,
        drawBufferSize,
        LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *touchInput = lv_indev_create();
    lv_indev_set_type(touchInput, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(touchInput, display);
    lv_indev_set_read_cb(touchInput, readTouch);

    ui_init();
    lv_label_set_text(objects.time, "--:--");
    lv_label_set_text(objects.current_screen, "Play");
    lv_obj_set_pos(objects.current_screen, 52, 12);
    lv_obj_set_size(objects.current_screen, 136, 18);
    lv_label_set_long_mode(objects.current_screen, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_align(objects.current_screen, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(objects.current_screen, &lv_font_turkish_14, LV_PART_MAIN);
    lv_obj_add_event_cb(objects.tab_view, onTabChanged, LV_EVENT_VALUE_CHANGED, nullptr);

    folderViewCreate();
    settingsViewCreate();
}

void guiStartTask() {
    xTaskCreatePinnedToCore(lvglTask, "LVGL", 10 * 1024, nullptr, 2, nullptr, 1);
}

GuiTab guiActiveTab() {
    return static_cast<GuiTab>(lv_tabview_get_tab_active(objects.tab_view));
}

void guiSetStatus(const char *text) {
    lv_label_set_text(objects.current_screen, text);
}

void guiSetLabelTextIfChanged(lv_obj_t *label, const char *text) {
    if (strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}
