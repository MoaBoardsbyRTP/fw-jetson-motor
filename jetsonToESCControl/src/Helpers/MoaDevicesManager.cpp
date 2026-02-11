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
{
}

MoaDevicesManager::~MoaDevicesManager() {
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
    // ESC arming sequence - set to minimum throttle immediately (no ramp)
    _esc.setThrottle(0);
}

void MoaDevicesManager::updateESC() {
    _esc.updateThrottle();
}

// === LED Indicators ===

void MoaDevicesManager::showBatteryLevel(MoaBattLevel level) {
    _leds.setBatteryLevel(level);
}

void MoaDevicesManager::indicateOverheat(bool active) {
    _leds.setTempLed(active);
}

void MoaDevicesManager::indicateOvercurrent(bool active) {
    _leds.setOvercurrentLed(active);
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
