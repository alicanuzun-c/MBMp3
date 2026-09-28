#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void publishMessage(void *) {
  for (;;) {
    Serial.println("Hello, World with FreeRTOS!");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  xTaskCreate(publishMessage, "PublishMessage", 2048, nullptr, 1, nullptr);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}