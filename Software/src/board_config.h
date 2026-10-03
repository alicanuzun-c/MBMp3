#pragma once

#include <stdint.h>

// TFT pinleri platformio.ini build_flags icinde tanimli (TFT_eSPI).

// Dokunmatik (XPT2046) kendi SPI hattinda
constexpr int touchIrqPin = 36;
constexpr int touchMosiPin = 32;
constexpr int touchMisoPin = 39;
constexpr int touchClockPin = 25;
constexpr int touchChipSelectPin = 33;

// SD kart yazilimsal SPI pinleri; TFT ve dokunmatikten bagimsiz
constexpr int sdChipSelectPin = 26;
constexpr uint8_t sdMisoPin = 19;
constexpr uint8_t sdMosiPin = 23;
constexpr uint8_t sdClockPin = 18;
