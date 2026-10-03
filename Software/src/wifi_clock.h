#pragma once

#include <time.h>

// wifi_passwords.h varsa Wi-Fi'ye baglanir; baglaninca saat NTP ile senkronize edilir.
void wifiClockBegin();

// Wi-Fi'yi tamamen kapatir. Saat senkronize olduysa RTC zamani tutmaya devam eder.
void wifiClockShutdown();

// Yerel saati verir; saat henuz senkronize olmadiysa false doner.
bool wifiClockLocalTime(struct tm &localTime);
