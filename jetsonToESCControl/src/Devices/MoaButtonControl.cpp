/**
 * @file MoaButtonControl.cpp
 * @brief Implementation of the MoaButtonControl class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaButtonControl.h"

// Global instance pointer for ISR access
MoaButtonControl* g_moaButtonControlInstance = nullptr;

// ISR trampoline
void IRAM_ATTR moaButtonControlISR() {
    if (g_moaButtonControlInstance != nullptr) {
        g_moaButtonControlInstance->handleInterrupt();
    }
}

MoaButtonControl::MoaButtonControl(QueueHandle_t eventQueue, MoaMcpDevice& mcpDevice, uint8_t intPin)
    : _eventQueue(eventQueue)
    , _mcpDevice(mcpDevice)
    , _intPin(intPin)
    , _interruptPending(false)
    , _debounceMs(MOA_BUTTON_DEFAULT_DEBOUNCE_MS)
    , _longPressMs(MOA_BUTTON_DEFAULT_LONG_PRESS_MS)
    , _longPressEnabled(false)
    , _lastRawState(0xFF)      // All released (active LOW, so 1 = released)
    , _debouncedState(0x00)    // All not pressed
    , _debounceUntil(0)        // No debounce active
{
    // Initialize button state tracking
    for (uint8_t i = 0; i < MOA_BUTTON_COUNT; i++) {
        _buttons[i].lastChangeTime = 0;
        _buttons[i].pressStartTime = 0;
        _buttons[i].isPressed = false;
        _buttons[i].longPressFired = false;
    }
}

MoaButtonControl::~MoaButtonControl() {
    // Detach interrupt if we were using it
    if (g_moaButtonControlInstance == this) {
        detachInterrupt(_intPin);
        g_moaButtonControlInstance = nullptr;
    }
}

void MoaButtonControl::begin(bool useInterrupt) {
    // Configure Port A pins 1-5 as inputs with pull-ups
    _mcpDevice.configurePortA(MOA_BUTTON_MASK, INPUT_PULLUP);
    
    if (useInterrupt) {
        // Enable interrupt-on-change for button pins
        _mcpDevice.enableInterruptPortA(MOA_BUTTON_MASK);
        
        // Configure ESP32 interrupt pin
        pinMode(_intPin, INPUT_PULLUP);
        
        // Set global instance for ISR access
        g_moaButtonControlInstance = this;
        
        // Attach interrupt
        attachInterrupt(digitalPinToInterrupt(_intPin), moaButtonControlISR, FALLING);
    }
    
    // Read initial state
    _lastRawState = _mcpDevice.readPortA();
}

void IRAM_ATTR MoaButtonControl::handleInterrupt() {
    _interruptPending = true;
}

void MoaButtonControl::processInterrupt() {
    // Atomically check and clear interrupt flag
    if (!__atomic_test_and_clear(&_interruptPending, __ATOMIC_SEQ_CST)) {
        return;
    }
    
    uint32_t now = millis();
    
    // Check if we're still in debounce window
    if (now < _debounceUntil) {
        // Still debouncing - ignore this interrupt but clear MCP interrupt
        // by reading the capture register
        _mcpDevice.readInterruptCapturePortA();
        return;
    }
    
    // Read INTCAPA to get state at interrupt time and clear MCP interrupt
    uint8_t capturedState = _mcpDevice.readInterruptCapturePortA();
    
    // Process only the buttons that changed (based on captured state vs last debounced)
    for (uint8_t i = 0; i < MOA_BUTTON_COUNT; i++) {
        uint8_t pin = BUTTON_PIN_STOP + i;
        bool capturedPressed = !(capturedState & (1 << pin));  // Active LOW
        
        // Only process if state actually changed from our debounced view
        if (capturedPressed != _buttons[i].isPressed) {
            processButtonFromInterrupt(i, capturedPressed, now);
        }
    }
    
    _lastRawState = capturedState;
    
    // Set debounce window - ignore further interrupts for debounce period
    _debounceUntil = now + _debounceMs;
}

void MoaButtonControl::processButtonFromInterrupt(uint8_t index, bool isPressed, uint32_t now) {
    ButtonState& btn = _buttons[index];
    
    // Update debounced state immediately (interrupt means real change)
    btn.lastChangeTime = now;
    btn.isPressed = isPressed;
    
    if (isPressed) {
        // Button just pressed
        btn.pressStartTime = now;
        btn.longPressFired = false;
        
        // Push press event
        pushButtonEvent(pinToCommandId(index), BUTTON_EVENT_PRESS);
        
        // Update debounced state bitmask
        _debouncedState |= (1 << index);
    } else {
        // Button just released
        // Optionally push release event (currently not used)
        // pushButtonEvent(pinToCommandId(index), BUTTON_EVENT_RELEASE);
        
        // Update debounced state bitmask
        _debouncedState &= ~(1 << index);
    }
}

bool MoaButtonControl::isInterruptPending() {
    return _interruptPending || (millis() < _debounceUntil);
}

void MoaButtonControl::checkLongPress() {
    if (!_longPressEnabled) {
        return;
    }
    
    uint32_t now = millis();
    
    // Check each button for long-press
    for (uint8_t i = 0; i < MOA_BUTTON_COUNT; i++) {
        ButtonState& btn = _buttons[i];
        
        if (btn.isPressed && !btn.longPressFired) {
            // Button is held - check for long press
            if ((now - btn.pressStartTime) >= _longPressMs) {
                btn.longPressFired = true;
                pushButtonEvent(pinToCommandId(i), BUTTON_EVENT_LONG_PRESS);
            }
        }
    }
}

void MoaButtonControl::update() {
    uint32_t now = millis();
    
    // Read current button state from MCP23018
    uint8_t rawState = _mcpDevice.readPortA();
    
    // Process each button (pins 1-5)
    for (uint8_t i = 0; i < MOA_BUTTON_COUNT; i++) {
        uint8_t pin = BUTTON_PIN_STOP + i;  // Pins 1, 2, 3, 4, 5
        bool rawPressed = !(rawState & (1 << pin));  // Active LOW
        
        processButton(i, rawPressed, now);
    }
    
    _lastRawState = rawState;
}

void MoaButtonControl::processButton(uint8_t index, bool isPressed, uint32_t now) {
    ButtonState& btn = _buttons[index];
    
    // Check if raw state changed
    bool stateChanged = (isPressed != btn.isPressed);
    
    if (stateChanged) {
        // State changed - check debounce
        if ((now - btn.lastChangeTime) >= _debounceMs) {
            // Debounce passed - accept new state
            btn.lastChangeTime = now;
            btn.isPressed = isPressed;
            
            if (isPressed) {
                // Button just pressed
                btn.pressStartTime = now;
                btn.longPressFired = false;
                
                // Push press event
                pushButtonEvent(pinToCommandId(index), BUTTON_EVENT_PRESS);
            } else {
                // Button just released
                // Optionally push release event (currently not used)
                // pushButtonEvent(pinToCommandId(index), BUTTON_EVENT_RELEASE);
            }
            
            // Update debounced state bitmask
            if (isPressed) {
                _debouncedState |= (1 << index);
            } else {
                _debouncedState &= ~(1 << index);
            }
        }
    } else if (btn.isPressed && _longPressEnabled && !btn.longPressFired) {
        // Button is held - check for long press
        if ((now - btn.pressStartTime) >= _longPressMs) {
            btn.longPressFired = true;
            pushButtonEvent(pinToCommandId(index), BUTTON_EVENT_LONG_PRESS);
        }
    }
}

void MoaButtonControl::setDebounceTime(uint32_t debounceMs) {
    _debounceMs = debounceMs;
}

uint32_t MoaButtonControl::getDebounceTime() const {
    return _debounceMs;
}

void MoaButtonControl::setLongPressTime(uint32_t longPressMs) {
    _longPressMs = longPressMs;
}

uint32_t MoaButtonControl::getLongPressTime() const {
    return _longPressMs;
}

void MoaButtonControl::enableLongPress(bool enable) {
    _longPressEnabled = enable;
}

bool MoaButtonControl::isLongPressEnabled() const {
    return _longPressEnabled;
}

bool MoaButtonControl::isButtonPressed(uint8_t buttonId) const {
    uint8_t index = commandIdToIndex(buttonId);
    if (index >= MOA_BUTTON_COUNT) {
        return false;
    }
    return _buttons[index].isPressed;
}

uint8_t MoaButtonControl::getButtonState() const {
    return _debouncedState;
}

uint32_t MoaButtonControl::getButtonHoldTime(uint8_t buttonId) const {
    uint8_t index = commandIdToIndex(buttonId);
    if (index >= MOA_BUTTON_COUNT || !_buttons[index].isPressed) {
        return 0;
    }
    return millis() - _buttons[index].pressStartTime;
}

uint8_t MoaButtonControl::getInterruptPin() const {
    return _intPin;
}

uint8_t MoaButtonControl::pinToCommandId(uint8_t pinIndex) const {
    // pinIndex 0-4 maps to COMMAND_BUTTON_STOP through COMMAND_BUTTON_100
    return COMMAND_BUTTON_STOP + pinIndex;
}

uint8_t MoaButtonControl::commandIdToIndex(uint8_t commandId) const {
    if (commandId < COMMAND_BUTTON_STOP || commandId > COMMAND_BUTTON_100) {
        return 0xFF;
    }
    return commandId - COMMAND_BUTTON_STOP;
}

void MoaButtonControl::pushButtonEvent(uint8_t commandId, uint8_t eventType) {
    if (_eventQueue == nullptr) {
        return;
    }

    ControlCommand cmd;
    cmd.controlType = CONTROL_TYPE_BUTTON;
    cmd.commandType = commandId;
    cmd.value = eventType;

    xQueueSend(_eventQueue, &cmd, 0);  // Don't block if queue is full
}
