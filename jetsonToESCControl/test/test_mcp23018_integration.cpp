#include <unity.h>
#include "MoaMcpDevice.h"

MoaMcpDevice* mcp;

void setUp(void) {
    // Called before each test
    mcp = new MoaMcpDevice(0x20);
}

void tearDown(void) {
    // Called after each test
    delete mcp;
}

void test_mcp23018_initialization() {
    bool success = mcp->begin();
    TEST_ASSERT_TRUE_MESSAGE(success, "MCP23018 failed to initialize");
    TEST_ASSERT_TRUE_MESSAGE(mcp->isInitialized(), "MCP23018 not marked as initialized");
}

void test_mcp23018_port_b_output() {
    TEST_ASSERT_TRUE(mcp->begin());
    
    // Configure Port B as outputs
    mcp->configurePortB(0xFF, OUTPUT);
    
    // Test pattern 1: All LEDs on
    mcp->writePortB(0x1F); // Only 5 LEDs connected (bits 0-4)
    delay(100);
    uint8_t readback = mcp->readPortB();
    TEST_ASSERT_EQUAL_MESSAGE(0x1F, readback, "Port B write/read failed");
    
    // Test pattern 2: All LEDs off
    mcp->writePortB(0x00);
    delay(100);
    readback = mcp->readPortB();
    TEST_ASSERT_EQUAL_MESSAGE(0x00, readback, "Port B clear failed");
}

void test_mcp23018_port_a_input() {
    TEST_ASSERT_TRUE(mcp->begin());
    
    // Configure Port A pins 1-5 as inputs with pull-ups
    mcp->configurePortA(0x3E, INPUT_PULLUP); // 0b00111110
    
    // Read initial state (should be high due to pull-ups)
    uint8_t initialState = mcp->readPortA();
    TEST_ASSERT_MESSAGE((initialState & 0x3E) == 0x3E, "Port A pull-ups failed");
}

void test_mcp23018_interrupt_setup() {
    TEST_ASSERT_TRUE(mcp->begin());
    
    // Enable interrupts on Port A pins 1-5
    mcp->enableInterruptPortA(0x3E);
    
    // Note: Can't easily test actual interrupts without button presses
    // Just verify the setup doesn't crash
    TEST_PASS();
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_mcp23018_initialization);
    RUN_TEST(test_mcp23018_port_b_output);
    RUN_TEST(test_mcp23018_port_a_input);
    RUN_TEST(test_mcp23018_interrupt_setup);
    
    return UNITY_END();
}
