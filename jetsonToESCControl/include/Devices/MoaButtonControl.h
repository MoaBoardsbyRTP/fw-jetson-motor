/**
 * @file MoaButtonControl.h
 * @brief Button input handling with debounce and long-press detection for MCP23018
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * This library provides button input handling via MCP23018 Port A with:
 * - Interrupt-driven or polled button reading
 * - Software debouncing with configurable timing
 * - Long-press detection for special mode entry
 * - Event-driven integration via FreeRTOS queue
 * 
 * ## Hardware Configuration
 * - Port A pins 1-5 connected to buttons (directly from schematic)
 * - MCP23018 INTA pin connected to ESP32 GPIO2
 * - Buttons active LOW (pressed = 0)
 */

#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "MoaMcpDevice.h"
#include "ControlCommand.h"

/**
 * @brief Button pin mapping on MCP23018 Port A
 */
#define BUTTON_PIN_STOP     1   // GPA1
#define BUTTON_PIN_25       2   // GPA2
#define BUTTON_PIN_50       3   // GPA3
#define BUTTON_PIN_75       4   // GPA4
#define BUTTON_PIN_100      5   // GPA5

/**
 * @brief Number of buttons
 */
#define MOA_BUTTON_COUNT    5

/**
 * @brief Bitmask of all button pins on Port A
 */
#define MOA_BUTTON_MASK     0x3E  // Bits 1-5 = 0b00111110

/**
 * @brief Default debounce time in milliseconds
 */
#define MOA_BUTTON_DEFAULT_DEBOUNCE_MS 50

/**
 * @brief Default long-press time in milliseconds
 */
#define MOA_BUTTON_DEFAULT_LONG_PRESS_MS      2000

/**
 * @brief Default very-long-press time in milliseconds
 */
#define MOA_BUTTON_DEFAULT_VERY_LONG_PRESS_MS 10000

/**
 * @brief Button input handler with debounce and long-press detection
 * 
 * MoaButtonControl provides button input handling via MCP23018 Port A with:
 * - Configurable software debouncing
 * - Optional long-press detection for special modes
 * - Interrupt-driven or polled operation
 * - Event-driven integration via FreeRTOS queue
 * 
 * When a button is pressed (or long-pressed), it automatically pushes a
 * ControlCommand event to the configured queue.
 * 
 * @note Buttons are active LOW (pressed = 0, released = 1)
 * @note Long-press events fire once when threshold is reached, not on release
 * 
 * ## Usage Example
 * @code
 * QueueHandle_t eventQueue = xQueueCreate(10, sizeof(ControlCommand));
 * MoaMcpDevice mcpDevice(0x20);
 * mcpDevice.begin();
 * 
 * MoaButtonControl buttons(eventQueue, mcpDevice, GPIO_NUM_2);
 * buttons.setDebounceTime(50);
 * buttons.setLongPressTime(5000);
 * buttons.enableLongPress(true);
 * buttons.begin();
 * 
 * // Option 1: Interrupt-driven (attach ISR to GPIO2)
 * // attachInterrupt(GPIO_NUM_2, buttonISR, FALLING);
 * // In ISR: buttons.handleInterrupt();
 * // In task: buttons.processInterrupt();
 * 
 * // Option 2: Polling (call periodically from task)
 * buttons.update();
 * 
 * // In ControlTask, handle the event:
 * // if (cmd.controlType == CONTROL_TYPE_BUTTON) {
 * //     if (cmd.commandType == COMMAND_BUTTON_STOP && cmd.value == BUTTON_EVENT_LONG_PRESS) {
 * //         enterConfigMode();
 * //     }
 * // }
 * @endcode
 */
class MoaButtonControl {
public:
    /**
     * @brief Construct a new MoaButtonControl object
     * 
     * @param eventQueue FreeRTOS queue handle to push button events to
     * @param mcpDevice Reference to shared MoaMcpDevice instance
     * @param intPin ESP32 GPIO pin connected to MCP23018 INTA (for interrupt mode)
     */
    MoaButtonControl(QueueHandle_t eventQueue, MoaMcpDevice& mcpDevice, uint8_t intPin);

    /**
     * @brief Destructor
     */
    ~MoaButtonControl();

    /**
     * @brief Initialize button inputs and optionally configure interrupts
     * 
     * Configures Port A pins as inputs with pull-ups and sets up
     * interrupt-on-change if interrupt mode is desired.
     * 
     * @param useInterrupt If true, configure MCP23018 for interrupt-on-change
     */
    void begin(bool useInterrupt = true);

    /**
     * @brief Handle interrupt from MCP23018 (call from ISR)
     * 
     * This method is safe to call from an ISR. It only sets a flag
     * indicating that button state should be read. Actual I2C communication
     * happens in processInterrupt().
     * 
     * @note Must be called from ISR attached to the interrupt pin
     */
    void IRAM_ATTR handleInterrupt();

    /**
     * @brief Process a pending interrupt
     * 
     * Reads INTCAPA register to get button state at interrupt time,
     * processes button changes with debounce, and clears MCP interrupt.
     * Call this from IOTask when isInterruptPending() returns true.
     */
    void processInterrupt();

    /**
     * @brief Check if an interrupt is pending or in debounce window
     * 
     * @return true if interrupt pending or debounce active
     * @return false if no activity
     */
    bool isInterruptPending();

    /**
     * @brief Check for long-press events (must be called periodically)
     * 
     * Polls button hold times and fires long-press events when threshold
     * is reached. This must be called periodically (e.g., from IOTask)
     * because long-press detection requires timing, not just edge detection.
     * 
     * @note Only works if enableLongPress(true) was called
     */
    void checkLongPress();

    /**
     * @brief Poll button state and generate events (alternative to interrupt mode)
     * 
     * Call this periodically from a task to poll button state.
     * Handles debouncing and long-press detection internally.
     * 
     * @note Can be used instead of or in addition to interrupt mode
     */
    void update();

    /**
     * @brief Set the debounce time
     * 
     * @param debounceMs Debounce time in milliseconds (default: 50ms)
     */
    void setDebounceTime(uint32_t debounceMs);

    /**
     * @brief Get the current debounce time
     * @return uint32_t Debounce time in milliseconds
     */
    uint32_t getDebounceTime() const;

    /**
     * @brief Set the long-press detection time
     * 
     * @param longPressMs Time in milliseconds to trigger long-press (default: 5000ms)
     */
    void setLongPressTime(uint32_t longPressMs);

    /**
     * @brief Get the current long-press time
     * @return uint32_t Long-press time in milliseconds
     */
    uint32_t getLongPressTime() const;

    /**
     * @brief Enable or disable long-press detection
     * 
     * @param enable True to enable long-press events
     */
    void enableLongPress(bool enable);

    /**
     * @brief Check if long-press detection is enabled
     * @return true if long-press detection is enabled
     */
    bool isLongPressEnabled() const;

    /**
     * @brief Set the very-long-press detection time
     * @param veryLongPressMs Time in milliseconds to trigger very-long-press (default: 10000ms)
     */
    void setVeryLongPressTime(uint32_t veryLongPressMs);

    /**
     * @brief Get the current very-long-press time
     * @return uint32_t Very-long-press time in milliseconds
     */
    uint32_t getVeryLongPressTime() const;

    /**
     * @brief Enable or disable very-long-press detection
     * @param enable True to enable very-long-press events
     */
    void enableVeryLongPress(bool enable);

    /**
     * @brief Check if very-long-press detection is enabled
     * @return true if very-long-press detection is enabled
     */
    bool isVeryLongPressEnabled() const;

    /**
     * @brief Check if a specific button is currently pressed
     * 
     * @param buttonId Button command ID (COMMAND_BUTTON_STOP, etc.)
     * @return true if button is pressed (debounced state)
     */
    bool isButtonPressed(uint8_t buttonId) const;

    /**
     * @brief Get the raw button state bitmask
     * 
     * @return uint8_t Bitmask of button states (bit set = pressed)
     */
    uint8_t getButtonState() const;

    /**
     * @brief Get how long a button has been held
     * 
     * @param buttonId Button command ID
     * @return uint32_t Hold time in milliseconds, or 0 if not pressed
     */
    uint32_t getButtonHoldTime(uint8_t buttonId) const;

    /**
     * @brief Get the interrupt pin number
     * @return uint8_t ESP32 GPIO pin number
     */
    uint8_t getInterruptPin() const;

    /**
     * @brief Set the event queue handle (must be called after queue creation)
     * @param eventQueue FreeRTOS queue handle for control events
     */
    void setEventQueue(QueueHandle_t eventQueue);

private:
    QueueHandle_t _eventQueue;         ///< Queue to push events to
    MoaMcpDevice& _mcpDevice;          ///< Reference to shared MCP device
    uint8_t _intPin;                   ///< ESP32 interrupt pin
    
    volatile bool _interruptPending;   ///< Flag set by ISR
    uint32_t _debounceMs;              ///< Debounce time
    uint32_t _longPressMs;             ///< Long-press threshold
    uint32_t _veryLongPressMs;         ///< Very-long-press threshold
    bool _longPressEnabled;            ///< Long-press detection enabled
    bool _veryLongPressEnabled;        ///< Very-long-press detection enabled
    
    uint8_t _lastRawState;             ///< Last raw reading from MCP
    uint8_t _debouncedState;           ///< Current debounced state
    uint32_t _debounceUntil;           ///< Timestamp until which to ignore interrupts

    /**
     * @brief Per-button state tracking
     */
    struct ButtonState {
        uint32_t lastChangeTime;       ///< Time of last state change (for debounce)
        uint32_t pressStartTime;       ///< Time when button was pressed
        bool isPressed;                ///< Current debounced pressed state
        bool longPressFired;           ///< Long-press event already sent
        bool veryLongPressFired;       ///< Very-long-press event already sent
    } _buttons[MOA_BUTTON_COUNT];

    /**
     * @brief Convert button pin index to command ID
     * @param pinIndex Pin index (0-4 for buttons 1-5)
     * @return uint8_t Command ID (COMMAND_BUTTON_*)
     */
    uint8_t pinToCommandId(uint8_t pinIndex) const;

    /**
     * @brief Convert command ID to button array index
     * @param commandId Command ID (COMMAND_BUTTON_*)
     * @return uint8_t Array index (0-4), or 0xFF if invalid
     */
    uint8_t commandIdToIndex(uint8_t commandId) const;

    /**
     * @brief Process a single button's state change (called from update())
     * @param index Button array index (0-4)
     * @param isPressed Current pressed state
     * @param now Current timestamp
     */
    void processButton(uint8_t index, bool isPressed, uint32_t now);

    /**
     * @brief Process a button state change from interrupt context
     * 
     * Similar to processButton but optimized for interrupt-driven operation.
     * Called from processInterrupt() with captured state from INTCAPA.
     * 
     * @param index Button array index (0-4)
     * @param isPressed Captured pressed state from INTCAPA
     * @param now Current timestamp
     */
    void processButtonFromInterrupt(uint8_t index, bool isPressed, uint32_t now);

    /**
     * @brief Push a button event to the queue
     * @param commandId Button command ID
     * @param eventType Event type (BUTTON_EVENT_*)
     */
    void pushButtonEvent(uint8_t commandId, uint8_t eventType);
};

/**
 * @brief Global pointer for ISR access (set during begin())
 * @note Only one MoaButtonControl instance can use interrupts at a time
 */
extern MoaButtonControl* g_moaButtonControlInstance;

/**
 * @brief ISR trampoline function for attachment to interrupt
 */
void IRAM_ATTR moaButtonControlISR();
