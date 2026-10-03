#include "wifi_clock.h"

#include <WiFi.h>

#if __has_include("wifi_passwords.h")
#include "wifi_passwords.h"
#else
#define MBMP3_WIFI_SSID ""
#define MBMP3_WIFI_PASSWORD ""
#endif

namespace {
bool ntpConfigured = false;

bool wifiCredentialsAvailable() {
    return MBMP3_WIFI_SSID[0] != '\0';
}
}

void wifiClockBegin() {
    if (!wifiCredentialsAvailable()) {
        WiFi.mode(WIFI_OFF);
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(MBMP3_WIFI_SSID, MBMP3_WIFI_PASSWORD);
}

void wifiClockShutdown() {
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
}

bool wifiClockLocalTime(struct tm &localTime) {
    if (WiFi.status() == WL_CONNECTED && !ntpConfigured) {
        configTzTime("TRT-3", "pool.ntp.org", "time.cloudflare.com");
        ntpConfigured = true;
    }

    return getLocalTime(&localTime, 10);
}
