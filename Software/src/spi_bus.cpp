#include "spi_bus.h"

#include <Arduino.h>

namespace {
SemaphoreHandle_t spiBusMutex;
}

void spiBusBegin() {
    spiBusMutex = xSemaphoreCreateRecursiveMutex();
}

void spiBusLock() {
    xSemaphoreTakeRecursive(spiBusMutex, portMAX_DELAY);
}

void spiBusUnlock() {
    xSemaphoreGiveRecursive(spiBusMutex);
}
