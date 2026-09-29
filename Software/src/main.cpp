#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static void counterTask(void* parameter) {
	uint32_t counter = 0;

	for (;;) {
		Serial.printf("Print Counter : %lu\n", static_cast<unsigned long>(counter++));
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void setup() {
	Serial.begin(115200);
	xTaskCreate(counterTask, "CounterTask", 2048, nullptr, 1, nullptr);
}

void loop() {

}