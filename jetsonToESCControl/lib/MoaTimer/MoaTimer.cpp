/**
 * @file MoaTimer.cpp
 * @brief Implementation of the MoaTimer class
 * @author Oscar Martinez
 * @date 2025-01-29
 */

#include "MoaTimer.h"

MoaTimer::MoaTimer(QueueHandle_t eventQueue, uint8_t timerId, const char* name)
    : _timerHandle(nullptr)
    , _eventQueue(eventQueue)
    , _timerId(timerId)
    , _durationMs(0)
    , _autoReload(false)
{
    // Create the timer in stopped state with a dummy period (will be set on start)
    // pdFALSE = one-shot by default, will be changed on start() if autoReload is true
    _timerHandle = xTimerCreate(
        name,                           // Timer name for debugging
        pdMS_TO_TICKS(1000),           // Dummy period, will be set on start()
        pdFALSE,                        // One-shot by default
        static_cast<void*>(this),       // Timer ID stores 'this' pointer
        timerCallback                   // Callback function
    );
}

MoaTimer::~MoaTimer() {
    if (_timerHandle != nullptr) {
        // Stop the timer first
        xTimerStop(_timerHandle, 0);
        // Delete the timer
        xTimerDelete(_timerHandle, portMAX_DELAY);
        _timerHandle = nullptr;
    }
}

bool MoaTimer::start(uint32_t durationMs, bool autoReload) {
    if (_timerHandle == nullptr) {
        return false;
    }

    _durationMs = durationMs;
    _autoReload = autoReload;

    // Stop timer if running
    if (xTimerIsTimerActive(_timerHandle)) {
        xTimerStop(_timerHandle, 0);
    }

    // Change the timer period
    if (xTimerChangePeriod(_timerHandle, pdMS_TO_TICKS(durationMs), portMAX_DELAY) != pdPASS) {
        return false;
    }

    // Set auto-reload mode
    // Note: FreeRTOS doesn't have a direct API to change auto-reload after creation
    // We handle this by recreating the timer or using vTimerSetReloadMode (FreeRTOS 10.3+)
#if (configUSE_TIMERS == 1)
    vTimerSetReloadMode(_timerHandle, autoReload ? pdTRUE : pdFALSE);
#endif

    // Start the timer
    return xTimerStart(_timerHandle, portMAX_DELAY) == pdPASS;
}

bool MoaTimer::stop() {
    if (_timerHandle == nullptr) {
        return false;
    }

    return xTimerStop(_timerHandle, portMAX_DELAY) == pdPASS;
}

bool MoaTimer::reset() {
    if (_timerHandle == nullptr) {
        return false;
    }

    // xTimerReset starts the timer if not running, or restarts if running
    return xTimerReset(_timerHandle, portMAX_DELAY) == pdPASS;
}

bool MoaTimer::isRunning() const {
    if (_timerHandle == nullptr) {
        return false;
    }

    return xTimerIsTimerActive(_timerHandle) != pdFALSE;
}

uint8_t MoaTimer::getTimerId() const {
    return _timerId;
}

uint32_t MoaTimer::getDuration() const {
    return _durationMs;
}

bool MoaTimer::setDuration(uint32_t durationMs) {
    _durationMs = durationMs;
    
    if (_timerHandle == nullptr) {
        return false;
    }

    // If timer is not running, just store the value (will be applied on next start)
    if (!isRunning()) {
        return true;
    }

    // If running, change the period (takes effect after current period expires)
    return xTimerChangePeriod(_timerHandle, pdMS_TO_TICKS(durationMs), portMAX_DELAY) == pdPASS;
}

void MoaTimer::timerCallback(TimerHandle_t xTimer) {
    // Retrieve the MoaTimer instance from the timer ID
    MoaTimer* instance = static_cast<MoaTimer*>(pvTimerGetTimerID(xTimer));
    
    if (instance != nullptr) {
        instance->pushTimerEvent();
    }
}

void MoaTimer::pushTimerEvent() {
    if (_eventQueue == nullptr) {
        return;
    }

    ControlCommand cmd;
    cmd.controlType = CONTROL_TYPE_TIMER;
    cmd.commandType = _timerId;  // Timer ID in commandType for routing
    cmd.value = 0;               // Available for future use

    // Use xQueueSendFromISR if called from ISR context, but FreeRTOS timer
    // callbacks run in the timer service task, so regular xQueueSend is fine
    xQueueSend(_eventQueue, &cmd, 0);  // Don't block if queue is full
}
