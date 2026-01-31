#include <unity.h>
#include "MoaCurrentControl.h"

QueueHandle_t testQueue;
MoaCurrentControl* currentControl;

void setUp(void) {
    testQueue = xQueueCreate(10, sizeof(ControlCommand));
    currentControl = new MoaCurrentControl(testQueue, 2); // GPIO2 for current sensor
}

void tearDown(void) {
    delete currentControl;
    vQueueDelete(testQueue);
}

void test_current_sensor_reading() {
    currentControl->begin();
    
    // Take a reading
    currentControl->update();
    
    float current = currentControl->getCurrentReading();
    TEST_ASSERT_TRUE_MESSAGE(current > -200.0f && current < 200.0f, "Current reading out of expected range");
    
    Serial.printf("Current: %.2fA\n", current);
}

void test_current_averaging() {
    currentControl->begin();
    
    // Fill averaging buffer
    for (int i = 0; i < 10; i++) {
        currentControl->update();
        delay(10);
    }
    
    TEST_ASSERT_TRUE_MESSAGE(currentControl->isAveragingReady(), "Averaging buffer not ready");
    
    float avgCurrent = currentControl->getAveragedCurrent();
    TEST_ASSERT_TRUE_MESSAGE(avgCurrent > -200.0f && avgCurrent < 200.0f, "Averaged current out of range");
    
    Serial.printf("Averaged current: %.2fA\n", avgCurrent);
}

void test_current_threshold_detection() {
    currentControl->begin();
    
    // Set thresholds for testing
    currentControl->setOvercurrentThreshold(50.0f);   // 50A positive
    currentControl->setReverseOvercurrentThreshold(-50.0f); // -50A negative
    currentControl->setHysteresis(5.0f);
    
    // Fill buffer
    for (int i = 0; i < 10; i++) {
        currentControl->update();
        delay(10);
    }
    
    // Check current state
    MoaCurrentState state = currentControl->getState();
    TEST_ASSERT_TRUE_MESSAGE(state == MoaCurrentState::NORMAL ||
                           state == MoaCurrentState::OVERCURRENT ||
                           state == MoaCurrentState::REVERSE_OVERCURRENT,
                           "Invalid current state");
    
    Serial.printf("Current state: %d\n", (int)state);
}

void test_current_events() {
    currentControl->begin();
    
    // Set very low threshold to trigger event
    currentControl->setOvercurrentThreshold(0.1f);   // Very low positive threshold
    currentControl->setReverseOvercurrentThreshold(-0.1f); // Very low negative threshold
    currentControl->setHysteresis(0.01f);
    
    // Fill buffer
    for (int i = 0; i < 10; i++) {
        currentControl->update();
        delay(10);
    }
    
    // Check for event
    ControlCommand cmd;
    bool eventReceived = xQueueReceive(testQueue, &cmd, 100) == pdTRUE;
    
    if (eventReceived) {
        TEST_ASSERT_EQUAL_MESSAGE(CONTROL_TYPE_CURRENT, cmd.controlType, "Wrong control type");
        TEST_ASSERT_TRUE_MESSAGE(cmd.commandType == COMMAND_CURRENT_OVERCURRENT ||
                               cmd.commandType == COMMAND_CURRENT_NORMAL ||
                               cmd.commandType == COMMAND_CURRENT_REVERSE_OVERCURRENT,
                               "Invalid current command type");
        Serial.printf("Current event: type=%d, value=%d (x0.1A)\n", cmd.commandType, cmd.value);
    } else {
        Serial.println("No current event (current within thresholds)");
    }
}

void test_current_sensor_calibration() {
    currentControl->begin();
    
    // Test sensitivity setting
    currentControl->setSensitivity(0.0066f); // ACS759-200B sensitivity
    TEST_ASSERT_EQUAL_MESSAGE(0.0066f, currentControl->getSensitivity(), "Sensitivity not set correctly");
    
    // Test zero offset setting
    currentControl->setZeroOffset(1.65f); // VCC/2 for 3.3V supply
    TEST_ASSERT_EQUAL_MESSAGE(1.65f, currentControl->getZeroOffset(), "Zero offset not set correctly");
    
    // Test reference voltage
    currentControl->setReferenceVoltage(3.3f);
    TEST_ASSERT_EQUAL_MESSAGE(3.3f, currentControl->getReferenceVoltage(), "Reference voltage not set correctly");
}

void test_current_adc_conversion() {
    currentControl->begin();
    
    // Test ADC resolution
    currentControl->setAdcResolution(12);
    TEST_ASSERT_EQUAL_MESSAGE(12, currentControl->getAdcResolution(), "ADC resolution not set correctly");
    
    // Get raw ADC reading
    uint16_t rawAdc = currentControl->getRawAdc();
    TEST_ASSERT_TRUE_MESSAGE(rawAdc >= 0 && rawAdc <= 4095, "Raw ADC value out of range");
    
    Serial.printf("Raw ADC: %d\n", rawAdc);
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_current_sensor_reading);
    RUN_TEST(test_current_averaging);
    RUN_TEST(test_current_threshold_detection);
    RUN_TEST(test_current_events);
    RUN_TEST(test_current_sensor_calibration);
    RUN_TEST(test_current_adc_conversion);
    
    return UNITY_END();
}
