#pragma once

#include <time.h>

// RTC'de gecerli saat yoksa ve wifi_passwords.h varsa Wi-Fi'ye baglanir, saati NTP'den alir
// ve Wi-Fi bellegini birakmak icin kart bir kez yeniden baslatilir. Saat gecerliyse
// (yazilimsal resetten sonra) Wi-Fi hic baslatilmaz.
void wifiClockBegin();

// Wi-Fi'yi tamamen kapatir. Saat senkronize olduysa RTC zamani tutmaya devam eder.
void wifiClockShutdown();

// Yerel saati verir; saat henuz senkronize olmadiysa false doner.
bool wifiClockLocalTime(struct tm &localTime);
