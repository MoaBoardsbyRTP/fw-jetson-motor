#include <unity.h>
#include "MoaLedControl.h"
#include "MoaMcpDevice.h"

MoaMcpDevice* mcp;
MoaLedControl* ledControl;

void setUp(void) {
    mcp = new MoaMcpDevice(0x20);
    ledControl = new MoaLedControl(*mcp);
}

void tearDown(void) {
    delete ledControl;
    delete mcp;
}

void test_led_initialization() {
    TEST_ASSERT_TRUE_MESSAGE(mcp->begin(), "MCP23018 failed to initialize");
    
    // Initialize LED control
    ledControl->begin();
    
    // All LEDs should be off initially
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_TEMP), "TEMP LED should be off initially");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_LOW), "BATT_LOW LED should be off initially");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_MED), "BATT_MED LED should be off initially");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_HI), "BATT_HI LED should be off initially");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_OVERCURRENT), "OVERCURRENT LED should be off initially");
}

void test_led_individual_control() {
    TEST_ASSERT_TRUE(mcp->begin());
    ledControl->begin();
    
    // Test turning on individual LEDs
    ledControl->setLed(LED_INDEX_TEMP, true);
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_TEMP), "TEMP LED should be on");
    
    ledControl->setLed(LED_INDEX_BATT_LOW, true);
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_LOW), "BATT_LOW LED should be on");
    
    // Test turning off LEDs
    ledControl->setLed(LED_INDEX_TEMP, false);
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_TEMP), "TEMP LED should be off");
    
    ledControl->setLed(LED_INDEX_BATT_LOW, false);
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_LOW), "BATT_LOW LED should be off");
}

void test_led_named_methods() {
    TEST_ASSERT_TRUE(mcp->begin());
    ledControl->begin();
    
    // Test named LED control methods
    ledControl->setTempLed(true);
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_TEMP), "TEMP LED should be on via named method");
    
    ledControl->setOvercurrentLed(true);
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_OVERCURRENT), "OVERCURRENT LED should be on via named method");
    
    ledControl->setTempLed(false);
    ledControl->setOvercurrentLed(false);
    
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_TEMP), "TEMP LED should be off");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_OVERCURRENT), "OVERCURRENT LED should be off");
}

void test_led_battery_level_display() {
    TEST_ASSERT_TRUE(mcp->begin());
    ledControl->begin();
    
    // Test battery level display
    ledControl->setBatteryLevel(MoaBattLevel::BATT_LOW);
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_LOW), "BATT_LOW LED should be on for LOW level");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_MED), "BATT_MED LED should be off for LOW level");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_HI), "BATT_HI LED should be off for LOW level");
    
    ledControl->setBatteryLevel(MoaBattLevel::BATT_MEDIUM);
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_LOW), "BATT_LOW LED should be on for MEDIUM level");
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_MED), "BATT_MED LED should be on for MEDIUM level");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_HI), "BATT_HI LED should be off for MEDIUM level");
    
    ledControl->setBatteryLevel(MoaBattLevel::BATT_HIGH);
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_LOW), "BATT_LOW LED should be on for HIGH level");
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_MED), "BATT_MED LED should be on for HIGH level");
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_HI), "BATT_HI LED should be on for HIGH level");
}

void test_led_batch_control() {
    TEST_ASSERT_TRUE(mcp->begin());
    ledControl->begin();
    
    // Test setting all LEDs with bitmask
    ledControl->setAllLeds(0x1F); // All 5 LEDs on
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_TEMP), "TEMP LED should be on");
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_LOW), "BATT_LOW LED should be on");
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_MED), "BATT_MED LED should be on");
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_HI), "BATT_HI LED should be on");
    TEST_ASSERT_TRUE_MESSAGE(ledControl->getLedState(LED_INDEX_OVERCURRENT), "OVERCURRENT LED should be on");
    
    // Test clear all
    ledControl->clearAllLeds();
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_TEMP), "TEMP LED should be off");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_LOW), "BATT_LOW LED should be off");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_MED), "BATT_MED LED should be off");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_BATT_HI), "BATT_HI LED should be off");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->getLedState(LED_INDEX_OVERCURRENT), "OVERCURRENT LED should be off");
    
    // Test all on
    ledControl->allLedsOn();
    uint8_t ledState = ledControl->getLedState(LED_INDEX_TEMP) && 
                      ledControl->getLedState(LED_INDEX_BATT_LOW) &&
                      ledControl->getLedState(LED_INDEX_BATT_MED) &&
                      ledControl->getLedState(LED_INDEX_BATT_HI) &&
                      ledControl->getLedState(LED_INDEX_OVERCURRENT);
    TEST_ASSERT_TRUE_MESSAGE(ledState, "All LEDs should be on");
}

void test_led_blink_patterns() {
    TEST_ASSERT_TRUE(mcp->begin());
    ledControl->begin();
    
    // Test starting blink on single LED
    ledControl->startBlink(LED_INDEX_TEMP, 200); // 200ms period
    TEST_ASSERT_TRUE_MESSAGE(ledControl->isBlinking(LED_INDEX_TEMP), "TEMP LED should be blinking");
    
    // Test stopping blink
    ledControl->stopBlink(LED_INDEX_TEMP);
    TEST_ASSERT_FALSE_MESSAGE(ledControl->isBlinking(LED_INDEX_TEMP), "TEMP LED should not be blinking");
    
    // Test blink pattern on multiple LEDs
    ledControl->startBlinkPattern(0x1F, 300); // All LEDs, 300ms period
    TEST_ASSERT_TRUE_MESSAGE(ledControl->isBlinking(LED_INDEX_TEMP), "TEMP LED should be blinking");
    TEST_ASSERT_TRUE_MESSAGE(ledControl->isBlinking(LED_INDEX_BATT_LOW), "BATT_LOW LED should be blinking");
    
    // Test stop all blinks
    ledControl->stopAllBlinks();
    TEST_ASSERT_FALSE_MESSAGE(ledControl->isBlinking(LED_INDEX_TEMP), "TEMP LED should not be blinking");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->isBlinking(LED_INDEX_BATT_LOW), "BATT_LOW LED should not be blinking");
}

void test_led_config_mode() {
    TEST_ASSERT_TRUE(mcp->begin());
    ledControl->begin();
    
    // Test config mode indication
    ledControl->setConfigModeIndication(true, 400); // 400ms blink period
    TEST_ASSERT_TRUE_MESSAGE(ledControl->isConfigModeActive(), "Config mode should be active");
    
    // All LEDs should be blinking in config mode
    TEST_ASSERT_TRUE_MESSAGE(ledControl->isBlinking(LED_INDEX_TEMP), "TEMP LED should be blinking in config mode");
    TEST_ASSERT_TRUE_MESSAGE(ledControl->isBlinking(LED_INDEX_BATT_LOW), "BATT_LOW LED should be blinking in config mode");
    
    // Test disabling config mode
    ledControl->setConfigModeIndication(false);
    TEST_ASSERT_FALSE_MESSAGE(ledControl->isConfigModeActive(), "Config mode should not be active");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->isBlinking(LED_INDEX_TEMP), "TEMP LED should not be blinking");
    TEST_ASSERT_FALSE_MESSAGE(ledControl->isBlinking(LED_INDEX_BATT_LOW), "BATT_LOW LED should not be blinking");
}

void test_led_update_timing() {
    TEST_ASSERT_TRUE(mcp->begin());
    ledControl->begin();
    
    // Start a fast blink to test update timing
    ledControl->startBlink(LED_INDEX_TEMP, 100); // 100ms period
    
    // Update should not crash and should handle blink timing
    for (int i = 0; i < 10; i++) {
        ledControl->update();
        delay(50); // Short delay to test timing
    }
    
    // LED should still be blinking
    TEST_ASSERT_TRUE_MESSAGE(ledControl->isBlinking(LED_INDEX_TEMP), "TEMP LED should still be blinking");
    
    Serial.println("LED timing update test completed");
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_led_initialization);
    RUN_TEST(test_led_individual_control);
    RUN_TEST(test_led_named_methods);
    RUN_TEST(test_led_battery_level_display);
    RUN_TEST(test_led_batch_control);
    RUN_TEST(test_led_blink_patterns);
    RUN_TEST(test_led_config_mode);
    RUN_TEST(test_led_update_timing);
    
    return UNITY_END();
}
