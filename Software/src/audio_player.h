#pragma once

#include <stddef.h>
#include <stdint.h>

enum class AudioPlayResult {
    Started,
    OpenFailed,
    NoMemory
};

// Bluetooth ve Wi-Fi heap'i tuketmeden once setup()'ta bir kez cagrilir:
// cozucu tamponlarini ayirir ve MP3 cozucu gorevini baslatir.
bool audioPlayerBegin();

// Calan parcayi birakir ve verilen dosyayi bastan calmaya baslar.
AudioPlayResult audioPlayerPlay(const char *path);

// 44.1 kHz stereo 16 bit PCM verir; Bluetooth gorevinden cagrilir.
size_t audioPlayerReadPcm(uint8_t *destination, size_t requestedBytes);

// Teshis: son cagridan beri Bluetooth'un istedigi ve verilebilen bayt sayisi; sayaclari sifirlar.
void audioPlayerTakeStats(uint32_t &requestedBytes, uint32_t &deliveredBytes);
