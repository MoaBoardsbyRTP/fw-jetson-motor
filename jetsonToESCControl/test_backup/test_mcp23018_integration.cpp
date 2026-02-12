#include <unity.h>
#include "MoaMcpDevice.h"
#include <Adafruit_MCP23XXX.h>

static MoaMcpDevice* mcp23018_mcp;

void setUp_mcp23018(void) {
    // Called before each test
    mcp23018_mcp = new MoaMcpDevice(0x20);
}

void tearDown_mcp23018(void) {
    // Called after each test
    delete mcp23018_mcp;
}

void test_mcp23018_initialization() {
    bool success = mcp23018_mcp->begin();
    TEST_ASSERT_TRUE_MESSAGE(success, "MCP23018 failed to initialize");
    TEST_ASSERT_TRUE_MESSAGE(mcp23018_mcp->isInitialized(), "MCP23018 not marked as initialized");
}

void test_mcp23018_port_b_output() {
    TEST_ASSERT_TRUE(mcp23018_mcp->begin());
    
    // Configure Port B pins 0-4 as outputs with pullups (open-drain)
    mcp23018_mcp->configurePortB(0x1F, OUTPUT, 0x1F);
    
    // Test pattern 1: All LEDs on
    mcp23018_mcp->writePortB(0x1F);
    delay(100);
    uint8_t readback = mcp23018_mcp->readPortB();
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x1F, readback & 0x1F, "Port B write/read failed");
    
    // Test pattern 2: All LEDs off
    mcp23018_mcp->writePortB(0x00);
    delay(100);
    readback = mcp23018_mcp->readPortB();
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x00, readback & 0x1F, "Port B clear failed");
}

void test_mcp23018_port_a_input() {
    TEST_ASSERT_TRUE(mcp23018_mcp->begin());
    
    // Configure Port A pins 1-5 as inputs with pull-ups using new overload
    mcp23018_mcp->configurePortA(0x3E, INPUT, 0x3E);
    
    // Read initial state (should be high due to pull-ups)
    uint8_t initialState = mcp23018_mcp->readPortA();
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x3E, initialState & 0x3E, "Port A pull-ups should read HIGH");
}

void test_mcp23018_output_with_pullup() {
    TEST_ASSERT_TRUE(mcp23018_mcp->begin());
    
    // Configure Port B pin 0 as output with pullup via single-pin API
    mcp23018_mcp->setPinMode(8, OUTPUT);  // B0
    Adafruit_MCP23X18& mcp = mcp23018_mcp->getMcp();
    mcp.setPullup(8, true);  // Enable pullup on output (MCP23018-specific)
    
    // Write HIGH and verify
    mcp23018_mcp->writePin(8, true);
    delay(10);
    bool val = mcp23018_mcp->readPin(8);
    TEST_ASSERT_TRUE_MESSAGE(val, "Output pin B0 with pullup should read HIGH");
}

void test_mcp23018_mixed_pullup_config() {
    TEST_ASSERT_TRUE(mcp23018_mcp->begin());
    
    // Configure Port B: pins 0-4 as outputs with pullups, pins 5-7 as inputs without pullups
    mcp23018_mcp->configurePortB(0x1F, OUTPUT, 0x1F);
    
    // Write a pattern and verify
    mcp23018_mcp->writePortB(0x0A);  // 0b00001010
    delay(10);
    uint8_t readback = mcp23018_mcp->readPortB();
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x0A, readback & 0x1F, "Mixed config pattern failed");
}

void test_mcp23018_interrupt_setup() {
    TEST_ASSERT_TRUE(mcp23018_mcp->begin());
    
    // Enable interrupts on Port A pins 1-5
    mcp23018_mcp->enableInterruptPortA(0x3E);
    
    // Note: Can't easily test actual interrupts without button presses
    // Just verify the setup doesn't crash
    TEST_PASS();
}

int main_mcp23018() {
    UNITY_BEGIN();
    
    RUN_TEST(test_mcp23018_initialization);
    RUN_TEST(test_mcp23018_port_b_output);
    RUN_TEST(test_mcp23018_port_a_input);
    RUN_TEST(test_mcp23018_output_with_pullup);
    RUN_TEST(test_mcp23018_mixed_pullup_config);
    RUN_TEST(test_mcp23018_interrupt_setup);
    
    return UNITY_END();
}

// Unity test runner
void setup() {
    delay(1000);
    Serial.begin(115200);
    main_mcp23018();
}

void loop() {
    // Empty
}
