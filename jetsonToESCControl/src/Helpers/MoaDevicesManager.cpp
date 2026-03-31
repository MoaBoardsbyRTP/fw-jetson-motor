/**
 * @file MoaDevicesManager.cpp
 * @brief Implementation of the MoaDevicesManager class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaDevicesManager.h"
#include "esp_log.h"

static const char* TAG = "Devices";

MoaDevicesManager::MoaDevicesManager(MoaLedControl& leds, ESCController& esc, MoaFlashLog& log,
                                     ConfigManager& config, MoaWiFiManager& wifiManager, MoaOTAManager& otaManager)
    : _leds(leds)
    , _esc(esc)
    , _log(log)
    , _config(config)
    , _wifiManager(wifiManager)
    , _otaManager(otaManager)
    , _eventQueue(nullptr)
    , _lastBattLevel(MoaBattLevel::BATT_HIGH)
    , _lastOverheat(false)
    , _lastOvercurrent(false)
    , _boardLocked(true)
    , _wifiConnectAnimTask(nullptr)
    , _wifiConnectAnimating(false)
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

void MoaDevicesManager::setThrottleLevel(uint16_t duty) {
    _esc.setThrottleDuty(duty);
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
    setThrottleLevel(_config.throttleLevel(commandType));

    if (commandType == COMMAND_BUTTON_100) {
        startTimer(TIMER_ID_FULL_THROTTLE, _config.escTime100);
    } else {
        uint32_t timeout = _config.throttleTimeout(commandType);
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
    setThrottleLevel(_config.escAfterFullThrottle);
    startTimer(TIMER_ID_THROTTLE, _config.escTimeAfterFullThrottle);
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
    }

    _timers[timerId]->start(durationMs);
    return true;
}

bool MoaDevicesManager::stopTimer(uint8_t timerId) {
    if (timerId >= MOA_TIMER_MAX_INSTANCES) {
        ESP_LOGW(TAG, "stopTimer: invalid timerId=%d", timerId);
        return false;
    }
    if (_timers[timerId] == nullptr) {
        return true;  // Already stopped/non-existent
    }

    _timers[timerId]->stop();
    return true;
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

void MoaDevicesManager::startOTA() {
    ESP_LOGI(TAG, "Starting WiFi STA + OTA");

    startWiFiConnectAnimation();
    bool wifiOk = _wifiManager.start();
    stopWiFiConnectAnimation();

    if (wifiOk) {
        ESP_LOGI(TAG, "WiFi connected at %s; starting OTA", _wifiManager.getIP().toString().c_str());
        _otaManager.begin();
    } else {
        ESP_LOGE(TAG, "Failed to connect WiFi STA");
    }
}

void MoaDevicesManager::stopOTA() {
    ESP_LOGI(TAG, "Stopping WiFi + OTA");
    stopWiFiConnectAnimation();
    _otaManager.stop();
    _wifiManager.stop();
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

void MoaDevicesManager::wifiConnectAnimTaskEntry(void* pvParameters) {
    auto* self = static_cast<MoaDevicesManager*>(pvParameters);
    while (self->_wifiConnectAnimating) {
        self->_leds.waveAllLeds(false);
        vTaskDelay(pdMS_TO_TICKS(800));
    }
    vTaskDelete(nullptr);
}

void MoaDevicesManager::startWiFiConnectAnimation() {
    if (_wifiConnectAnimating) {
        return;
    }

    _wifiConnectAnimating = true;
    BaseType_t ok = xTaskCreatePinnedToCore(
        wifiConnectAnimTaskEntry,
        "WiFiConnAnim",
        2048,
        this,
        1,
        &_wifiConnectAnimTask,
        0
    );

    if (ok != pdPASS) {
        _wifiConnectAnimating = false;
        _wifiConnectAnimTask = nullptr;
        ESP_LOGW(TAG, "Could not start WiFi connect animation task");
    }
}

void MoaDevicesManager::stopWiFiConnectAnimation() {
    if (!_wifiConnectAnimating) {
        return;
    }

    _wifiConnectAnimating = false;
    vTaskDelay(pdMS_TO_TICKS(20));
    _wifiConnectAnimTask = nullptr;
}
