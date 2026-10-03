#include "wifi_clock.h"

#include <WiFi.h>

#if __has_include("wifi_passwords.h")
#include "wifi_passwords.h"
#else
#define MBMP3_WIFI_SSID ""
#define MBMP3_WIFI_PASSWORD ""
#endif

namespace {
constexpr char timeZone[] = "TRT-3";
// Bu tarihten eski saat "henuz senkronize olmadi" sayilir.
constexpr time_t validTimeThreshold = 1700000000;
// Kullanici cihazi kullanmaya baslamissa saat geldi diye yeniden baslatma.
constexpr uint32_t restartWindowMs = 60000;
constexpr uint32_t restartMarkerValue = 0x5C10C4A1;

// Wi-Fi kapatildiktan sonra bile ~17 KB heap geri gelmez (ag yigini ve olay gorevleri).
// Bu yuzden saat alininca bir kez yeniden baslatilir: RTC zamanlayicisi yazilimsal
// resette zamani korur, sonraki acilista Wi-Fi hic baslatilmaz.
// RTC_NOINIT, yeniden baslatmadan sonra saat yine gecersizse dongu olusmasini engeller.
RTC_NOINIT_ATTR uint32_t restartMarker;

bool ntpConfigured = false;
bool wifiActive = false;
bool restartAfterSync = false;

bool wifiCredentialsAvailable() {
    return MBMP3_WIFI_SSID[0] != '\0';
}

bool systemTimeValid() {
    return time(nullptr) > validTimeThreshold;
}
}

void wifiClockBegin() {
    setenv("TZ", timeZone, 1);
    tzset();

    const bool restartedForSync = restartMarker == restartMarkerValue;
    restartMarker = 0;

    if (systemTimeValid()) {
        Serial.println("[SAAT] RTC'de gecerli saat var, Wi-Fi baslatilmiyor");
        WiFi.mode(WIFI_OFF);
        return;
    }

    if (!wifiCredentialsAvailable()) {
        WiFi.mode(WIFI_OFF);
        return;
    }

    restartAfterSync = !restartedForSync;
    wifiActive = true;
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(MBMP3_WIFI_SSID, MBMP3_WIFI_PASSWORD);
}

void wifiClockShutdown() {
    restartAfterSync = false;
    if (!wifiActive) {
        return;
    }
    wifiActive = false;
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
}

bool wifiClockLocalTime(struct tm &localTime) {
    if (wifiActive && WiFi.status() == WL_CONNECTED && !ntpConfigured) {
        configTzTime(timeZone, "pool.ntp.org", "time.cloudflare.com");
        ntpConfigured = true;
    }

    if (!systemTimeValid()) {
        return false;
    }

    if (restartAfterSync) {
        restartAfterSync = false;
        if (millis() < restartWindowMs) {
            Serial.println("[SAAT] saat alindi, Wi-Fi bellegini birakmak icin yeniden baslatiliyor");
            restartMarker = restartMarkerValue;
            WiFi.disconnect(true, false);
            delay(100);
            esp_restart();
        }
    }

    return getLocalTime(&localTime, 0);
}
