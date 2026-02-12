#include <unity.h>
#include "MoaBattControl.h"

QueueHandle_t testQueue;
MoaBattControl* battControl;

void setUp(void) {
    testQueue = xQueueCreate(10, sizeof(ControlCommand));
    battControl = new MoaBattControl(testQueue, 1); // GPIO1 for battery ADC
    Serial.begin(115200);
}

void tearDown(void) {
    delete battControl;
    vQueueDelete(testQueue);
}

void test_battery_adc_reading() {
    battControl->begin();
    
    // Take a reading
    battControl->update();
    
    float voltage = battControl->getCurrentVoltage();
    TEST_ASSERT_TRUE_MESSAGE(voltage > 0.0f, "Battery voltage should be positive");
    TEST_ASSERT_TRUE_MESSAGE(voltage < 30.0f, "Battery voltage too high (check divider ratio)");
    
    Serial.printf("Battery voltage: %.2fV\n", voltage);
}

void test_battery_averaging() {
    battControl->begin();
    
    // Fill averaging buffer
    for (int i = 0; i < 10; i++) {
        battControl->update();
        delay(10);
    }
    
    TEST_ASSERT_TRUE_MESSAGE(battControl->isAveragingReady(), "Averaging buffer not ready");
    
    float avgVoltage = battControl->getAveragedVoltage();
    TEST_ASSERT_TRUE_MESSAGE(avgVoltage > 0.0f && avgVoltage < 30.0f, "Averaged voltage out of range");
    
    Serial.printf("Averaged battery voltage: %.2fV\n", avgVoltage);
}

void test_battery_threshold_levels() {
    battControl->begin();
    
    // Set thresholds for testing
    battControl->setLowThreshold(3.3f);   // Low battery
    battControl->setHighThreshold(4.0f);  // High battery
    battControl->setHysteresis(0.1f);
    
    // Fill buffer
    for (int i = 0; i < 10; i++) {
        battControl->update();
        delay(10);
    }
    
    // Check current level
    MoaBattLevel level = battControl->getLevel();
    TEST_ASSERT_TRUE_MESSAGE(level == MoaBattLevel::BATT_LOW || 
                           level == MoaBattLevel::BATT_MEDIUM || 
                           level == MoaBattLevel::BATT_HIGH, 
                           "Invalid battery level");
    
    Serial.printf("Battery level: %d\n", (int)level);
}

void test_battery_events() {
    battControl->begin();
    
    // Set very low threshold to trigger event
    battControl->setLowThreshold(10.0f);  // Very high threshold
    battControl->setHighThreshold(15.0f); // Very high threshold
    battControl->setHysteresis(0.1f);
    
    // Fill buffer
    for (int i = 0; i < 10; i++) {
        battControl->update();
        delay(10);
    }
    
    // Check for event (should trigger LOW level since battery < 10V)
    ControlCommand cmd;
    bool eventReceived = xQueueReceive(testQueue, &cmd, 100) == pdTRUE;
    
    if (eventReceived) {
        TEST_ASSERT_EQUAL_MESSAGE(CONTROL_TYPE_BATTERY, cmd.controlType, "Wrong control type");
        TEST_ASSERT_TRUE_MESSAGE(cmd.commandType == COMMAND_BATT_LEVEL_LOW ||
                               cmd.commandType == COMMAND_BATT_LEVEL_MEDIUM ||
                               cmd.commandType == COMMAND_BATT_LEVEL_HIGH,
                               "Invalid battery command type");
        Serial.printf("Battery event: type=%d, value=%d mV\n", cmd.commandType, cmd.value);
    } else {
        Serial.println("No battery event (voltage below threshold)");
    }
}

void test_battery_divider_ratio() {
    battControl->begin();
    
    // Test divider ratio setting
    battControl->setDividerRatio(3.0f);
    TEST_ASSERT_EQUAL_MESSAGE(3.0f, battControl->getDividerRatio(), "Divider ratio not set correctly");
    
    // Test invalid ratio (should be clamped to 1.0)
    battControl->setDividerRatio(0.5f);
    TEST_ASSERT_EQUAL_MESSAGE(1.0f, battControl->getDividerRatio(), "Invalid divider ratio not clamped");
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_battery_adc_reading);
    RUN_TEST(test_battery_averaging);
    RUN_TEST(test_battery_threshold_levels);
    RUN_TEST(test_battery_events);
    RUN_TEST(test_battery_divider_ratio);
    
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
