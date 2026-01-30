/**
 * @file MoaLedControl.cpp
 * @brief Implementation of the MoaLedControl class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaLedControl.h"

MoaLedControl::MoaLedControl(MoaMcpDevice& mcpDevice)
    : _mcpDevice(mcpDevice)
    , _ledState(0x00)
    , _blinkMask(0x00)
    , _configModeActive(false)
{
    // Initialize blink state for all LEDs
    for (uint8_t i = 0; i < MOA_LED_COUNT; i++) {
        _blinkState[i].period = MOA_LED_DEFAULT_BLINK_PERIOD_MS;
        _blinkState[i].lastToggleTime = 0;
        _blinkState[i].phase = false;
    }
}

MoaLedControl::~MoaLedControl() {
    // Nothing to clean up
}

void MoaLedControl::begin() {
    // Configure Port B pins 0-4 as outputs
    _mcpDevice.configurePortB(MOA_LED_MASK, OUTPUT);
    
    // Turn all LEDs off
    clearAllLeds();
}

void MoaLedControl::update() {
    uint32_t now = millis();
    bool stateChanged = false;
    
    // Update each blinking LED
    for (uint8_t i = 0; i < MOA_LED_COUNT; i++) {
        if (_blinkMask & (1 << i)) {
            uint32_t elapsed = now - _blinkState[i].lastToggleTime;
            uint32_t halfPeriod = _blinkState[i].period / 2;
            
            if (elapsed >= halfPeriod) {
                // Toggle phase
                _blinkState[i].phase = !_blinkState[i].phase;
                _blinkState[i].lastToggleTime = now;
                
                // Update LED state based on phase
                if (_blinkState[i].phase) {
                    _ledState |= (1 << i);
                } else {
                    _ledState &= ~(1 << i);
                }
                stateChanged = true;
            }
        }
    }
    
    // Write to hardware if any blink state changed
    if (stateChanged) {
        writeLedState();
    }
}

void MoaLedControl::setLed(uint8_t ledIndex, bool state) {
    if (ledIndex >= MOA_LED_COUNT) {
        return;
    }
    
    // Stop blinking this LED if it was blinking
    _blinkMask &= ~(1 << ledIndex);
    
    // Update state
    if (state) {
        _ledState |= (1 << ledIndex);
    } else {
        _ledState &= ~(1 << ledIndex);
    }
    
    writeLedState();
}

void MoaLedControl::toggleLed(uint8_t ledIndex) {
    if (ledIndex >= MOA_LED_COUNT) {
        return;
    }
    
    // Stop blinking this LED if it was blinking
    _blinkMask &= ~(1 << ledIndex);
    
    // Toggle state
    _ledState ^= (1 << ledIndex);
    
    writeLedState();
}

bool MoaLedControl::getLedState(uint8_t ledIndex) const {
    if (ledIndex >= MOA_LED_COUNT) {
        return false;
    }
    return (_ledState & (1 << ledIndex)) != 0;
}

void MoaLedControl::setTempLed(bool state) {
    setLed(LED_INDEX_TEMP, state);
}

void MoaLedControl::setBattLowLed(bool state) {
    setLed(LED_INDEX_BATT_LOW, state);
}

void MoaLedControl::setBattMedLed(bool state) {
    setLed(LED_INDEX_BATT_MED, state);
}

void MoaLedControl::setBattHiLed(bool state) {
    setLed(LED_INDEX_BATT_HI, state);
}

void MoaLedControl::setOvercurrentLed(bool state) {
    setLed(LED_INDEX_OVERCURRENT, state);
}

void MoaLedControl::setBatteryLevel(MoaBattLevel level) {
    // Stop any blinking on battery LEDs
    _blinkMask &= ~((1 << LED_INDEX_BATT_LOW) | (1 << LED_INDEX_BATT_MED) | (1 << LED_INDEX_BATT_HI));
    
    // Clear battery LED bits first
    _ledState &= ~((1 << LED_INDEX_BATT_LOW) | (1 << LED_INDEX_BATT_MED) | (1 << LED_INDEX_BATT_HI));
    
    switch (level) {
        case MoaBattLevel::LOW:
            // Only LOW LED on
            _ledState |= (1 << LED_INDEX_BATT_LOW);
            break;
            
        case MoaBattLevel::MEDIUM:
            // LOW and MED LEDs on
            _ledState |= (1 << LED_INDEX_BATT_LOW) | (1 << LED_INDEX_BATT_MED);
            break;
            
        case MoaBattLevel::HIGH:
            // All three battery LEDs on
            _ledState |= (1 << LED_INDEX_BATT_LOW) | (1 << LED_INDEX_BATT_MED) | (1 << LED_INDEX_BATT_HI);
            break;
    }
    
    writeLedState();
}

void MoaLedControl::setAllLeds(uint8_t mask) {
    // Stop all blinking
    _blinkMask = 0x00;
    
    // Apply mask (only lower 5 bits)
    _ledState = mask & MOA_LED_MASK;
    
    writeLedState();
}

void MoaLedControl::clearAllLeds() {
    // Stop all blinking
    _blinkMask = 0x00;
    _configModeActive = false;
    
    _ledState = 0x00;
    writeLedState();
}

void MoaLedControl::allLedsOn() {
    // Stop all blinking
    _blinkMask = 0x00;
    
    _ledState = MOA_LED_MASK;
    writeLedState();
}

void MoaLedControl::startBlink(uint8_t ledIndex, uint32_t periodMs) {
    if (ledIndex >= MOA_LED_COUNT) {
        return;
    }
    
    _blinkState[ledIndex].period = periodMs;
    _blinkState[ledIndex].lastToggleTime = millis();
    _blinkState[ledIndex].phase = true;  // Start with LED on
    
    // Set LED on for first phase
    _ledState |= (1 << ledIndex);
    
    // Mark as blinking
    _blinkMask |= (1 << ledIndex);
    
    writeLedState();
}

void MoaLedControl::stopBlink(uint8_t ledIndex) {
    if (ledIndex >= MOA_LED_COUNT) {
        return;
    }
    
    _blinkMask &= ~(1 << ledIndex);
    // Leave LED in current state
}

void MoaLedControl::stopBlink(uint8_t ledIndex, bool finalState) {
    if (ledIndex >= MOA_LED_COUNT) {
        return;
    }
    
    _blinkMask &= ~(1 << ledIndex);
    
    // Set final state
    if (finalState) {
        _ledState |= (1 << ledIndex);
    } else {
        _ledState &= ~(1 << ledIndex);
    }
    
    writeLedState();
}

void MoaLedControl::startBlinkPattern(uint8_t mask, uint32_t periodMs) {
    uint32_t now = millis();
    
    for (uint8_t i = 0; i < MOA_LED_COUNT; i++) {
        if (mask & (1 << i)) {
            _blinkState[i].period = periodMs;
            _blinkState[i].lastToggleTime = now;
            _blinkState[i].phase = true;
            
            // Set LED on for first phase
            _ledState |= (1 << i);
        }
    }
    
    // Mark all as blinking
    _blinkMask |= (mask & MOA_LED_MASK);
    
    writeLedState();
}

void MoaLedControl::stopAllBlinks() {
    _blinkMask = 0x00;
    _configModeActive = false;
    // Leave LEDs in current state
}

bool MoaLedControl::isBlinking(uint8_t ledIndex) const {
    if (ledIndex >= MOA_LED_COUNT) {
        return false;
    }
    return (_blinkMask & (1 << ledIndex)) != 0;
}

void MoaLedControl::setBlinkPeriod(uint8_t ledIndex, uint32_t periodMs) {
    if (ledIndex >= MOA_LED_COUNT) {
        return;
    }
    _blinkState[ledIndex].period = periodMs;
}

void MoaLedControl::setConfigModeIndication(bool enabled, uint32_t periodMs) {
    _configModeActive = enabled;
    
    if (enabled) {
        // Start all LEDs blinking together
        startBlinkPattern(MOA_LED_MASK, periodMs);
    } else {
        // Stop all blinking and turn LEDs off
        stopAllBlinks();
        clearAllLeds();
    }
}

bool MoaLedControl::isConfigModeActive() const {
    return _configModeActive;
}

void MoaLedControl::writeLedState() {
    _mcpDevice.writePortB(_ledState);
}
