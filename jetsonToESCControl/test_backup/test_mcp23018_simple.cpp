#include <unity.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23XXX.h>

// Simple test without MoaMcpDevice wrapper
static Adafruit_MCP23XXX mcp;

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
    
    // Configure Port B as outputs
    mcp.pinMode(8, OUTPUT);  // B0
    mcp.pinMode(9, OUTPUT);  // B1
    mcp.pinMode(10, OUTPUT); // B2
    mcp.pinMode(11, OUTPUT); // B3
    mcp.pinMode(12, OUTPUT); // B4
    
    // Test pattern 1: All LEDs on
    mcp.digitalWrite(8, HIGH);
    mcp.digitalWrite(9, HIGH);
    mcp.digitalWrite(10, HIGH);
    mcp.digitalWrite(11, HIGH);
    mcp.digitalWrite(12, HIGH);
    delay(100);
    
    // Test pattern 2: All LEDs off
    mcp.digitalWrite(8, LOW);
    mcp.digitalWrite(9, LOW);
    mcp.digitalWrite(10, LOW);
    mcp.digitalWrite(11, LOW);
    mcp.digitalWrite(12, LOW);
    delay(100);
    
    TEST_PASS();
}

void test_mcp23018_port_a_input() {
    TEST_ASSERT_TRUE(mcp.begin_I2C(0x20));
    
    // Configure Port A pins 1-5 as inputs with pull-ups
    mcp.pinMode(1, INPUT_PULLUP);  // A1
    mcp.pinMode(2, INPUT_PULLUP);  // A2
    mcp.pinMode(3, INPUT_PULLUP);  // A3
    mcp.pinMode(4, INPUT_PULLUP);  // A4
    mcp.pinMode(5, INPUT_PULLUP);  // A5
    
    // Read initial state (should be high due to pull-ups)
    int state1 = mcp.digitalRead(1);
    int state2 = mcp.digitalRead(2);
    int state3 = mcp.digitalRead(3);
    int state4 = mcp.digitalRead(4);
    int state5 = mcp.digitalRead(5);
    
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, state1, "Pin A1 should be HIGH with pull-up");
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, state2, "Pin A2 should be HIGH with pull-up");
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, state3, "Pin A3 should be HIGH with pull-up");
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, state4, "Pin A4 should be HIGH with pull-up");
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, state5, "Pin A5 should be HIGH with pull-up");
}

int main() {
    UNITY_BEGIN();
    
    delay(1000); // Wait for serial
    
    RUN_TEST(test_mcp23018_basic_init);
    RUN_TEST(test_mcp23018_port_b_output);
    RUN_TEST(test_mcp23018_port_a_input);
    
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
