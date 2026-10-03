#include <Arduino.h>

#include "audio_player.h"
#include "bluetooth.h"
#include "gui.h"
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
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Teshis: Bluetooth ses istiyorsa ne kadarini karsilayabildigimizi 5 saniyede bir yaz.
    uint32_t requestedBytes = 0;
    uint32_t deliveredBytes = 0;
    audioPlayerTakeStats(requestedBytes, deliveredBytes);
    if (requestedBytes > 0) {
        Serial.printf("[SES] 5 sn: istenen %u B, verilen %u B (%u%%), bos heap: %u\n",
                      requestedBytes, deliveredBytes,
                      static_cast<unsigned>(static_cast<uint64_t>(deliveredBytes) * 100 / requestedBytes),
                      ESP.getFreeHeap());
    }
}
