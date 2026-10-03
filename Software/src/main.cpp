#include <Arduino.h>

#include "audio_player.h"
#include "bluetooth.h"
#include "gui.h"
#include "spi_bus.h"
#include "storage.h"
#include "wifi_clock.h"

namespace {
void logResetReason() {
    static const char *const resetReasons[] = {
        "bilinmiyor", "guc acildi", "harici pin", "yazilim", "panic (cokme)",
        "kesme watchdog", "gorev watchdog", "watchdog", "uyku", "brownout (voltaj dustu)", "SDIO"};
    const esp_reset_reason_t resetReason = esp_reset_reason();
    Serial.printf("\n[BOOT] reset nedeni: %s\n",
                  resetReason <= ESP_RST_SDIO ? resetReasons[resetReason] : "?");
}
}

void setup() {
    Serial.begin(115200);
    logResetReason();

    spiBusBegin();
    storageBegin();
    guiBegin();

    // Cozucu bellegi Wi-Fi ve Bluetooth heap'i tuketmeden once ayrilmali.
    if (!audioPlayerBegin()) {
        Serial.println("[MP3] cozucu icin bellek ayrilamadi");
    }
    Serial.printf("[MP3] cozucu hazir, bos heap: %u\n", ESP.getFreeHeap());
    bluetoothSetPcmSource(audioPlayerReadPcm);

    wifiClockBegin();
    guiStartTask();
    Serial.printf("[BOOT] arayuz ve Wi-Fi hazir, bos heap: %u\n", ESP.getFreeHeap());
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
