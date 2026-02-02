#include <Arduino.h>
#include <Adafruit_MCP23X18.h>

#define LED_PIN 8  // Built-in LED on Beetle ESP32-C3

TaskHandle_t fadeLedTaskHandle = NULL;
TaskHandle_t printTaskHandle = NULL;
TaskHandle_t readMCP23X18TaskHandle = NULL;


void fadeLedTask(void *pvParameters){
	ledcSetup(0, 1000, 12);
	ledcAttachPin(LED_PIN, 0);
	int brightness = 20;
	int direction = 10;
	for(;;){
		ledcWrite(0, brightness);
		vTaskDelay(1 / portTICK_PERIOD_MS);
		if (brightness <= 10 || brightness >= 4070) 
			direction = -direction;
		brightness += direction;
		Serial.printf("Brightness: %d\n", brightness);
	}
}

void printTask(void *pvParameters){
	for(;;){
		Serial.println("Hello World from FreeRTOS!");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void readMCP23018Task(void *pvParameters){
  Adafruit_MCP23X18 mcp;
  mcp.begin_I2C();
  mcp.enableAddrPins();
  for(;;){
    Serial.printf("MCP23018: %d\n", mcp.readGPIOAB());
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello World from FreeRTOS!");
  
  xTaskCreatePinnedToCore(
    fadeLedTask,   // Task function
    "FadeLedTask", // Task name
    2048,             // Stack size (bytes)
    NULL,             // Parameters
    1,                // Priority
    &fadeLedTaskHandle, 
    0                 // Core (0 for ESP32-C3 single core)
  );

  xTaskCreatePinnedToCore(
    printTask,
    "PrintTask",
    2048,
    NULL,
    1,
    &printTaskHandle,
    0
  );
/*
  xTaskCreatePinnedToCore(
    readMCP23018Task,
    "ReadMCPTask",
    2048,
    NULL,
    1,
    &readMCP23X18TaskHandle,
    0
  );*/
}

void loop() {
}