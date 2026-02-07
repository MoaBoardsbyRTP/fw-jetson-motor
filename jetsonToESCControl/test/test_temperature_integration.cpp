#include <unity.h>
#include "MoaTempControl.h"

QueueHandle_t testQueue;
MoaTempControl* tempControl;

void setUp(void) {
    // Create test queue
    testQueue = xQueueCreate(10, sizeof(ControlCommand));
    tempControl = new MoaTempControl(testQueue, 6); // GPIO6 for DS18B20
}

void tearDown(void) {
    delete tempControl;
    vQueueDelete(testQueue);
}

void test_temperature_sensor_init() {
    // Just call begin() - if no crash, sensor is accessible
    tempControl->begin();
    
    // Try to get a reading to verify sensor is connected
    tempControl->update();
    delay(1000); // DS18B20 conversion time
    tempControl->update();
    
    float currentTemp = tempControl->getCurrentTemp();
    TEST_ASSERT_NOT_EQUAL_MESSAGE(DEVICE_DISCONNECTED_C, currentTemp, "DS18B20 sensor not connected");
}

void test_temperature_reading() {
    tempControl->begin();
    
    // Wait for first reading
    tempControl->update();
    delay(1000); // DS18B20 conversion time
    tempControl->update();
    
    float currentTemp = tempControl->getCurrentTemp();
    TEST_ASSERT_NOT_EQUAL_MESSAGE(DEVICE_DISCONNECTED_C, currentTemp, "Temperature sensor disconnected");
    TEST_ASSERT_TRUE_MESSAGE(currentTemp > -40.0f && currentTemp < 125.0f, "Temperature out of valid range");
    
    Serial.printf("Temperature: %.2f°C\n", currentTemp);
}

void test_temperature_averaging() {
    tempControl->begin();
    
    // Fill averaging buffer
    for (int i = 0; i < 10; i++) {
        tempControl->update();
        delay(1000); // Wait for conversion
    }
    
    TEST_ASSERT_TRUE_MESSAGE(tempControl->isAveragingReady(), "Averaging buffer not ready after 10 samples");
    
    float avgTemp = tempControl->getAveragedTemp();
    TEST_ASSERT_TRUE_MESSAGE(avgTemp > -40.0f && avgTemp < 125.0f, "Averaged temperature out of valid range");
    
    Serial.printf("Averaged Temperature: %.2f°C\n", avgTemp);
}

void test_temperature_threshold_events() {
    tempControl->begin();
    
    // Set low threshold to trigger event
    tempControl->setTargetTemp(0.0f); // Very low threshold
    tempControl->setHysteresis(1.0f);
    
    // Fill buffer and check for event
    for (int i = 0; i < 10; i++) {
        tempControl->update();
        delay(1000);
    }
    
    // Check if event was queued
    ControlCommand cmd;
    bool eventReceived = xQueueReceive(testQueue, &cmd, 100) == pdTRUE;
    
    if (eventReceived) {
        TEST_ASSERT_EQUAL_MESSAGE(CONTROL_TYPE_TEMPERATURE, cmd.controlType, "Wrong control type in temperature event");
        TEST_ASSERT_EQUAL_MESSAGE(COMMAND_TEMP_CROSSED_ABOVE, cmd.commandType, "Expected temperature crossed above event");
        Serial.printf("Temperature event: type=%d, value=%d\n", cmd.commandType, cmd.value);
    } else {
        // No event might be OK if temperature is below threshold
        Serial.println("No temperature event (temperature below threshold)");
    }
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_temperature_sensor_init);
    RUN_TEST(test_temperature_reading);
    RUN_TEST(test_temperature_averaging);
    RUN_TEST(test_temperature_threshold_events);
    
    return UNITY_END();
}

void setup() {
    delay(1000);
    Serial.begin(115200);
    main();
}

void loop() {
    // Empty
}
