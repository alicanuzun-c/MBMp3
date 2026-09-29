#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
uint16_t calData[5] = { 300, 3800, 200, 3900, 2 };

void drawTouchInfo(int x, int y) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 20);
    tft.println("Touch test");

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, 80);
    tft.print("X: ");
    tft.println(x);

    tft.setCursor(10, 120);
    tft.print("Y: ");
    tft.println(y);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(10, 180);
    tft.println("Basma noktasi");

    tft.drawCircle(x, y, 8, TFT_RED);
    tft.drawCircle(x, y, 12, TFT_RED);
}

static void touchTask(void*) {
    Serial.println("Touch test task baslatildi");

#if defined(TFT_BL)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif

    tft.init();
    tft.setRotation(1);
    tft.setTouch(calData);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 20);
    tft.println("Ekrana bas");

    while (true) {
        uint16_t x = 0, y = 0;
        if (tft.getTouch(&x, &y)) {
            Serial.print("Touch => X:");
            Serial.print(x);
            Serial.print(" Y:");
            Serial.println(y);
            drawTouchInfo(x, y);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    xTaskCreate(touchTask, "touchTask", 8192, nullptr, 2, nullptr);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}