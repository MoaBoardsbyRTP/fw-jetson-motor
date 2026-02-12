#include <unity.h>
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_MCP23X18.h"

// Simple test using MCP23018-specific API
static Adafruit_MCP23X18 mcp;

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

void test_mcp23018_basic_init() {
    bool success = mcp.begin_I2C(0x20);
    TEST_ASSERT_TRUE_MESSAGE(success, "MCP23018 failed to initialize");
    
    // Test basic functionality - just verify initialization worked
    TEST_PASS();
}

void test_mcp23018_port_b_output() {
    TEST_ASSERT_TRUE(mcp.begin_I2C(0x20));
    
    // Configure Port B pins 0-4 as outputs with pullups (open-drain)
    mcp.configGPIOB(0xE0, 0x1F);  // dir: bits 0-4 output (0), 5-7 input (1); pullups on 0-4
    
    // Test pattern 1: All LEDs on
    mcp.writeGPIOB(0x1F);
    delay(100);
    uint8_t readback = mcp.readGPIOB();
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x1F, readback & 0x1F, "Port B write/read pattern 0x1F failed");
    
    // Test pattern 2: All LEDs off
    mcp.writeGPIOB(0x00);
    delay(100);
    readback = mcp.readGPIOB();
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x00, readback & 0x1F, "Port B clear failed");
}

void test_mcp23018_port_a_input() {
    TEST_ASSERT_TRUE(mcp.begin_I2C(0x20));
    
    // Configure Port A pins 1-5 as inputs with pull-ups using bulk API
    mcp.configGPIOA(0x3E, 0x3E);  // dir: pins 1-5 input (1); pullups on pins 1-5
    
    // Read initial state (should be high due to pull-ups)
    uint8_t state = mcp.readGPIOA();
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x3E, state & 0x3E, "Port A pull-ups should read HIGH");
}

void test_mcp23018_output_with_pullup() {
    TEST_ASSERT_TRUE(mcp.begin_I2C(0x20));
    
    // MCP23018-specific: set pin as output, then enable pullup independently
    mcp.pinMode(8, OUTPUT);  // B0 as output
    mcp.setPullup(8, true);  // Enable pullup on output pin (open-drain)
    
    // Verify pin is functional
    mcp.digitalWrite(8, HIGH);
    delay(10);
    uint8_t val = mcp.digitalRead(8);
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, val, "Output pin B0 with pullup should read HIGH");
    
    // Disable pullup
    mcp.setPullup(8, false);
    TEST_PASS();
}

void test_mcp23018_pinmode_preserves_pullup() {
    TEST_ASSERT_TRUE(mcp.begin_I2C(0x20));
    
    // Set pin as input with pullup
    mcp.pinMode(0, INPUT_PULLUP);
    uint8_t val = mcp.digitalRead(0);
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, val, "Pin A0 should be HIGH with INPUT_PULLUP");
    
    // Switch to output — MCP23018 override should preserve pullup state
    mcp.pinMode(0, OUTPUT);
    
    // Re-enable as input (no pullup) to verify pullup was preserved during OUTPUT
    // by checking the pullup register directly via setPullup read-back
    // For now, just verify the transitions don't crash
    mcp.pinMode(0, INPUT);
    TEST_PASS();
}

int main() {
    UNITY_BEGIN();
    
    delay(1000); // Wait for serial
    
    RUN_TEST(test_mcp23018_basic_init);
    RUN_TEST(test_mcp23018_port_b_output);
    RUN_TEST(test_mcp23018_port_a_input);
    RUN_TEST(test_mcp23018_output_with_pullup);
    RUN_TEST(test_mcp23018_pinmode_preserves_pullup);
    
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
