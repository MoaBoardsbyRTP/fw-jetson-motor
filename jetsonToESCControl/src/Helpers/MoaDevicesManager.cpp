/**
 * @file MoaDevicesManager.cpp
 * @brief Implementation of the MoaDevicesManager class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaDevicesManager.h"
#include "Constants.h"
#include "esp_log.h"

static const char* TAG = "Devices";

MoaDevicesManager::MoaDevicesManager(MoaLedControl& leds, ESCController& esc, MoaFlashLog& log)
    : _leds(leds)
    , _esc(esc)
    , _log(log)
    , _eventQueue(nullptr)
    , _lastBattLevel(MoaBattLevel::BATT_HIGH)
    , _lastOverheat(false)
    , _lastOvercurrent(false)
    , _boardLocked(true)
{
    memset(_timers, 0, sizeof(_timers));
}

MoaDevicesManager::~MoaDevicesManager() {
    for (int i = 0; i < MOA_TIMER_MAX_INSTANCES; i++) {
        if (_timers[i] != nullptr) {
            delete _timers[i];
            _timers[i] = nullptr;
        }
    }
}

// === ESC Control ===

void MoaDevicesManager::setThrottleLevel(uint8_t percent) {
    _esc.setThrottlePercent(percent);
}

void MoaDevicesManager::stopMotor() {
    ESP_LOGI(TAG, "Motor stop");
    _esc.stop();
}

void MoaDevicesManager::armESC() {
    ESP_LOGI(TAG, "ESC arming");
    _esc.stop();
}

void MoaDevicesManager::updateESC() {
    _esc.updateThrottle();
}

void MoaDevicesManager::engageThrottle(uint8_t commandType) {
    stopTimer(TIMER_ID_THROTTLE);
    stopTimer(TIMER_ID_FULL_THROTTLE);
    setThrottleLevel(escThrottleLevel(commandType));

    if (commandType == COMMAND_BUTTON_100) {
        startTimer(TIMER_ID_FULL_THROTTLE, ESC_100_TIME);
    } else {
        uint32_t timeout = escThrottleTimeout(commandType);
        if (timeout > 0) {
            startTimer(TIMER_ID_THROTTLE, timeout);
        }
    }
}

void MoaDevicesManager::disengageThrottle() {
    stopTimer(TIMER_ID_THROTTLE);
    stopTimer(TIMER_ID_FULL_THROTTLE);
    stopMotor();
}

void MoaDevicesManager::handleThrottleStepDown() {
    setThrottleLevel(ESC_BREAKING_MODE);
    startTimer(TIMER_ID_THROTTLE, ESC_75_TIME_100);
}

// === Timer Management ===

void MoaDevicesManager::setEventQueue(QueueHandle_t queue) {
    _eventQueue = queue;
}

bool MoaDevicesManager::startTimer(uint8_t timerId, uint32_t durationMs) {
    if (timerId >= MOA_TIMER_MAX_INSTANCES) {
        ESP_LOGW(TAG, "startTimer: invalid timerId=%d", timerId);
        return false;
    }
    if (_eventQueue == nullptr) {
        ESP_LOGW(TAG, "startTimer: event queue not set");
        return false;
    }

    // Lazy-create the timer on first use
    if (_timers[timerId] == nullptr) {
        _timers[timerId] = new MoaTimer(_eventQueue, timerId);
        ESP_LOGI(TAG, "Timer %d created", timerId);
    }

    return _timers[timerId]->start(durationMs);
}

bool MoaDevicesManager::stopTimer(uint8_t timerId) {
    if (timerId >= MOA_TIMER_MAX_INSTANCES) {
        return false;
    }
    if (_timers[timerId] == nullptr) {
        return true;  // Not created = not running
    }
    return _timers[timerId]->stop();
}

bool MoaDevicesManager::isTimerRunning(uint8_t timerId) const {
    if (timerId >= MOA_TIMER_MAX_INSTANCES || _timers[timerId] == nullptr) {
        return false;
    }
    return _timers[timerId]->isRunning();
}

// === LED Indicators ===

void MoaDevicesManager::showBatteryLevel(MoaBattLevel level) {
    _lastBattLevel = level;
    _leds.setBatteryLevel(level);
}

void MoaDevicesManager::indicateOverheat(bool active) {
    _lastOverheat = active;
    if (active) {
        _leds.startBlink(LED_INDEX_TEMP, LED_WARNING_BLINK_MS);
    } else {
        _leds.stopBlink(LED_INDEX_TEMP, false);
    }
}

void MoaDevicesManager::indicateOvercurrent(bool active) {
    _lastOvercurrent = active;
    if (active) {
        _leds.startBlink(LED_INDEX_OVERCURRENT, LED_WARNING_BLINK_MS);
    } else {
        // Restore locked/unlocked solid state
        _leds.stopBlink(LED_INDEX_OVERCURRENT, _boardLocked);
    }
}

void MoaDevicesManager::showBoardLocked() {
    _boardLocked = true;
    if (!_lastOvercurrent) {
        _leds.setOvercurrentLed(true);
    }
}

void MoaDevicesManager::showBoardUnlocked() {
    _boardLocked = false;
    if (!_lastOvercurrent) {
        _leds.setOvercurrentLed(false);
    }
}

void MoaDevicesManager::clearWarnings() {
    _leds.setTempLed(false);
    _leds.setOvercurrentLed(false);
}

void MoaDevicesManager::enterConfigMode() {
    ESP_LOGI(TAG, "Entering config mode");
    _leds.setConfigModeIndication(true, LED_CONFIG_BLINK_MS);
}

void MoaDevicesManager::exitConfigMode() {
    ESP_LOGI(TAG, "Exiting config mode");
    _leds.setConfigModeIndication(false);
}

void MoaDevicesManager::allLedsOff() {
    _leds.clearAllLeds();
}

void MoaDevicesManager::waveAllLeds(bool fast) {
    _leds.waveAllLeds(fast);
}

void MoaDevicesManager::refreshLedIndicators() {
    _leds.setBatteryLevel(_lastBattLevel);
    if (_lastOverheat) {
        _leds.startBlink(LED_INDEX_TEMP, LED_WARNING_BLINK_MS);
    } else {
        _leds.stopBlink(LED_INDEX_TEMP, false);
    }
    if (_lastOvercurrent) {
        _leds.startBlink(LED_INDEX_OVERCURRENT, LED_WARNING_BLINK_MS);
    } else {
        _leds.setOvercurrentLed(_boardLocked);
    }
}

// === Logging ===

void MoaDevicesManager::logSystem(uint8_t code) {
    _log.logSystem(code);
}

void MoaDevicesManager::logButton(uint8_t code) {
    _log.logButton(code);
}

void MoaDevicesManager::logTemp(uint8_t code, int16_t tempX10) {
    _log.logTemp(code, tempX10);
}

void MoaDevicesManager::logBatt(uint8_t code, int16_t voltageMv) {
    _log.logBatt(code, voltageMv);
}

void MoaDevicesManager::logCurrent(uint8_t code, int16_t currentX10) {
    _log.logCurrent(code, currentX10);
}

void MoaDevicesManager::logState(uint8_t code) {
    _log.logState(code);
}

void MoaDevicesManager::logError(uint8_t code, int16_t value) {
    _log.logError(code, value);
}

void MoaDevicesManager::updateLog() {
    _log.update();
}
