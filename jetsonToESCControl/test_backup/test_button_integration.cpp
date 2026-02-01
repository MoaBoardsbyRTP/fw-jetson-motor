#include <unity.h>
#include "MoaButtonControl.h"
#include "MoaMcpDevice.h"

QueueHandle_t testQueue;
MoaMcpDevice* mcp;
MoaButtonControl* buttonControl;

void setUp(void) {
    testQueue = xQueueCreate(10, sizeof(ControlCommand));
    mcp = new MoaMcpDevice(0x20);
    buttonControl = new MoaButtonControl(testQueue, *mcp, 2); // GPIO2 for interrupt
}

void tearDown(void) {
    delete buttonControl;
    delete mcp;
    vQueueDelete(testQueue);
}

void test_button_initialization() {
    TEST_ASSERT_TRUE_MESSAGE(mcp->begin(), "MCP23018 failed to initialize");
    
    // Initialize button control (non-interrupt mode for testing)
    buttonControl->begin(false); // Don't use interrupts for basic test
    
    // Check initial state (all buttons should be released)
    TEST_ASSERT_FALSE_MESSAGE(buttonControl->isButtonPressed(COMMAND_BUTTON_STOP), "STOP button should not be pressed initially");
    TEST_ASSERT_FALSE_MESSAGE(buttonControl->isButtonPressed(COMMAND_BUTTON_25), "25% button should not be pressed initially");
    TEST_ASSERT_FALSE_MESSAGE(buttonControl->isButtonPressed(COMMAND_BUTTON_50), "50% button should not be pressed initially");
    TEST_ASSERT_FALSE_MESSAGE(buttonControl->isButtonPressed(COMMAND_BUTTON_75), "75% button should not be pressed initially");
    TEST_ASSERT_FALSE_MESSAGE(buttonControl->isButtonPressed(COMMAND_BUTTON_100), "100% button should not be pressed initially");
}

void test_button_polling_mode() {
    TEST_ASSERT_TRUE(mcp->begin());
    buttonControl->begin(false); // Polling mode
    
    // Read button state
    buttonControl->update();
    
    // Get button state bitmask
    uint8_t buttonState = buttonControl->getButtonState();
    TEST_ASSERT_MESSAGE(buttonState >= 0 && buttonState <= 0x1F, "Button state bitmask out of range");
    
    Serial.printf("Button state bitmask: 0x%02X\n", buttonState);
}

void test_button_debounce_settings() {
    buttonControl->begin(false);
    
    // Test debounce time setting
    buttonControl->setDebounceTime(100);
    TEST_ASSERT_EQUAL_MESSAGE(100, buttonControl->getDebounceTime(), "Debounce time not set correctly");
    
    // Test long press time setting
    buttonControl->setLongPressTime(3000);
    TEST_ASSERT_EQUAL_MESSAGE(3000, buttonControl->getLongPressTime(), "Long press time not set correctly");
    
    // Test long press enable/disable
    buttonControl->enableLongPress(true);
    TEST_ASSERT_TRUE_MESSAGE(buttonControl->isLongPressEnabled(), "Long press should be enabled");
    
    buttonControl->enableLongPress(false);
    TEST_ASSERT_FALSE_MESSAGE(buttonControl->isLongPressEnabled(), "Long press should be disabled");
}

void test_button_interrupt_mode() {
    TEST_ASSERT_TRUE(mcp->begin());
    
    // Initialize with interrupts
    buttonControl->begin(true); // Use interrupts
    
    // Check interrupt pin configuration
    uint8_t intPin = buttonControl->getInterruptPin();
    TEST_ASSERT_EQUAL_MESSAGE(2, intPin, "Interrupt pin should be GPIO2");
    
    // Note: Can't easily test actual interrupts without physical button presses
    // Just verify setup doesn't crash
    TEST_PASS();
}

void test_button_hold_time() {
    TEST_ASSERT_TRUE(mcp->begin());
    buttonControl->begin(false);
    
    // Test hold time for non-pressed button (should return 0)
    uint32_t holdTime = buttonControl->getButtonHoldTime(COMMAND_BUTTON_STOP);
    TEST_ASSERT_EQUAL_MESSAGE(0, holdTime, "Hold time should be 0 for non-pressed button");
    
    // Test invalid button ID (should return 0)
    holdTime = buttonControl->getButtonHoldTime(99);
    TEST_ASSERT_EQUAL_MESSAGE(0, holdTime, "Hold time should be 0 for invalid button");
}

void test_button_events_manual() {
    TEST_ASSERT_TRUE(mcp->begin());
    buttonControl->begin(false);
    
    // Clear any existing events
    ControlCommand cmd;
    while (xQueueReceive(testQueue, &cmd, 0) == pdPASS) {
        // Clear queue
    }
    
    // Simulate button press by manually calling update
    // Note: This test is limited without actual button hardware
    buttonControl->update();
    
    // Check if any events were generated (unlikely without button presses)
    bool eventReceived = xQueueReceive(testQueue, &cmd, 100) == pdTRUE;
    
    if (eventReceived) {
        TEST_ASSERT_EQUAL_MESSAGE(CONTROL_TYPE_BUTTON, cmd.controlType, "Wrong control type");
        TEST_ASSERT_TRUE_MESSAGE(cmd.commandType >= COMMAND_BUTTON_STOP && cmd.commandType <= COMMAND_BUTTON_100,
                               "Invalid button command type");
        Serial.printf("Button event: type=%d, value=%d\n", cmd.commandType, cmd.value);
    } else {
        Serial.println("No button events (expected without button presses)");
    }
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_button_initialization);
    RUN_TEST(test_button_polling_mode);
    RUN_TEST(test_button_debounce_settings);
    RUN_TEST(test_button_interrupt_mode);
    RUN_TEST(test_button_hold_time);
    RUN_TEST(test_button_events_manual);
    
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
