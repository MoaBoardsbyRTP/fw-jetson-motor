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
    
    // Configure Port B as outputs
    mcp23018_mcp->configurePortB(0xFF, OUTPUT);
    
    // Test pattern 1: All LEDs on
    mcp23018_mcp->writePortB(0x1F); // Only 5 LEDs connected (bits 0-4)
    delay(100);
    uint8_t readback = mcp23018_mcp->readPortB();
    TEST_ASSERT_EQUAL_MESSAGE(0x1F, readback, "Port B write/read failed");
    
    // Test pattern 2: All LEDs off
    mcp23018_mcp->writePortB(0x00);
    delay(100);
    readback = mcp23018_mcp->readPortB();
    TEST_ASSERT_EQUAL_MESSAGE(0x00, readback, "Port B clear failed");
}

void test_mcp23018_port_a_input() {
    TEST_ASSERT_TRUE(mcp23018_mcp->begin());
    
    // Configure Port A pins 1-5 as inputs with pull-ups
    mcp23018_mcp->configurePortA(0x3E, INPUT_PULLUP); // 0b00111110
    
    // Read initial state (should be high due to pull-ups)
    uint8_t initialState = mcp23018_mcp->readPortA();
    TEST_ASSERT_MESSAGE((initialState & 0x3E) == 0x3E, "Port A pull-ups failed");
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
