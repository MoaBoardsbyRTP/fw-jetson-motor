/**
 * @file ConfigManager.cpp
 * @brief Implementation of the ConfigManager class
 * @author Oscar Martinez
 * @date 2026-02-13
 */

#include "ConfigManager.h"
#include "MoaBattControl.h"
#include "MoaCurrentControl.h"
#include "MoaTempControl.h"
#include "ESCController.h"
#include "ControlCommand.h"
#include "esp_log.h"

static const char* TAG = "Config";

ConfigManager::ConfigManager() {
    loadDefaults();
}

void ConfigManager::loadDefaults() {
    // Surfing timers
    escTime25       = ESC_25_TIME;
    escTime50       = ESC_50_TIME;
    escTime75       = ESC_75_TIME;
    escTime100      = ESC_100_TIME;
    escTime75From100 = ESC_75_TIME_100;

    // Throttle percentages
    escEcoMode      = ESC_ECO_MODE;
    escPaddleMode   = ESC_PADDLE_MODE;
    escBreakingMode = ESC_BREAKING_MODE;
    escFullThrottle = ESC_FULL_THROTTLE_MODE;
    escRampRate     = ESC_RAMP_RATE;

    // Battery
    battHigh        = BATT_THRESHOLD_HIGH;
    battMedium      = BATT_THRESHOLD_MEDIUM;
    battLow         = BATT_THRESHOLD_LOW;
    battHysteresis  = BATT_HYSTERESIS;

    // Temperature
    tempTarget      = TEMP_THRESHOLD_TARGET;
    tempHysteresis  = TEMP_HYSTERESIS;

    // Current
    currentOvercurrent = CURRENT_THRESHOLD_OVERCURRENT;
    currentReverse     = CURRENT_THRESHOLD_REVERSE;
    currentHysteresis  = CURRENT_HYSTERESIS;
}

void ConfigManager::begin() {
    Preferences prefs;
    if (!prefs.begin(CONFIG_NVS_NAMESPACE, true)) {  // read-only
        ESP_LOGW(TAG, "NVS open failed, using defaults");
        return;
    }

    // Surfing timers
    escTime25        = prefs.getULong("esc_t25",     ESC_25_TIME);
    escTime50        = prefs.getULong("esc_t50",     ESC_50_TIME);
    escTime75        = prefs.getULong("esc_t75",     ESC_75_TIME);
    escTime100       = prefs.getULong("esc_t100",    ESC_100_TIME);
    escTime75From100 = prefs.getULong("esc_t75_100", ESC_75_TIME_100);

    // Throttle percentages
    escEcoMode       = prefs.getUChar("esc_eco",     ESC_ECO_MODE);
    escPaddleMode    = prefs.getUChar("esc_paddle",  ESC_PADDLE_MODE);
    escBreakingMode  = prefs.getUChar("esc_break",   ESC_BREAKING_MODE);
    escFullThrottle  = prefs.getUChar("esc_full",    ESC_FULL_THROTTLE_MODE);
    escRampRate      = prefs.getFloat("esc_ramp",    ESC_RAMP_RATE);

    // Battery
    battHigh         = prefs.getFloat("batt_high",   BATT_THRESHOLD_HIGH);
    battMedium       = prefs.getFloat("batt_med",    BATT_THRESHOLD_MEDIUM);
    battLow          = prefs.getFloat("batt_low",    BATT_THRESHOLD_LOW);
    battHysteresis   = prefs.getFloat("batt_hyst",   BATT_HYSTERESIS);

    // Temperature
    tempTarget       = prefs.getFloat("temp_tgt",    TEMP_THRESHOLD_TARGET);
    tempHysteresis   = prefs.getFloat("temp_hyst",   TEMP_HYSTERESIS);

    // Current
    currentOvercurrent = prefs.getFloat("curr_oc",   CURRENT_THRESHOLD_OVERCURRENT);
    currentReverse     = prefs.getFloat("curr_rev",  CURRENT_THRESHOLD_REVERSE);
    currentHysteresis  = prefs.getFloat("curr_hyst", CURRENT_HYSTERESIS);

    prefs.end();

    ESP_LOGI(TAG, "Settings loaded from NVS");
    ESP_LOGD(TAG, "  Batt: high=%.2fV, med=%.2fV, low=%.2fV, hyst=%.2fV", battHigh, battMedium, battLow, battHysteresis);
    ESP_LOGD(TAG, "  Temp: target=%.1fC, hyst=%.1fC", tempTarget, tempHysteresis);
    ESP_LOGD(TAG, "  Current: OC=%.1fA, rev=%.1fA, hyst=%.1fA", currentOvercurrent, currentReverse, currentHysteresis);
    ESP_LOGD(TAG, "  ESC: eco=%d%%, paddle=%d%%, break=%d%%, full=%d%%, ramp=%.1f%%/s",
             escEcoMode, escPaddleMode, escBreakingMode, escFullThrottle, escRampRate);
    ESP_LOGD(TAG, "  Timers: t25=%lums, t50=%lums, t75=%lums, t100=%lums, t75from100=%lums",
             escTime25, escTime50, escTime75, escTime100, escTime75From100);
}

bool ConfigManager::save() {
    Preferences prefs;
    if (!prefs.begin(CONFIG_NVS_NAMESPACE, false)) {  // read-write
        ESP_LOGE(TAG, "NVS open for write failed");
        return false;
    }

    bool ok = true;

    // Surfing timers
    ok &= (prefs.putULong("esc_t25",     escTime25)        > 0);
    ok &= (prefs.putULong("esc_t50",     escTime50)        > 0);
    ok &= (prefs.putULong("esc_t75",     escTime75)        > 0);
    ok &= (prefs.putULong("esc_t100",    escTime100)       > 0);
    ok &= (prefs.putULong("esc_t75_100", escTime75From100) > 0);

    // Throttle percentages
    ok &= (prefs.putUChar("esc_eco",     escEcoMode)       > 0);
    ok &= (prefs.putUChar("esc_paddle",  escPaddleMode)    > 0);
    ok &= (prefs.putUChar("esc_break",   escBreakingMode)  > 0);
    ok &= (prefs.putUChar("esc_full",    escFullThrottle)  > 0);
    ok &= (prefs.putFloat("esc_ramp",    escRampRate)      > 0);

    // Battery
    ok &= (prefs.putFloat("batt_high",   battHigh)         > 0);
    ok &= (prefs.putFloat("batt_med",    battMedium)       > 0);
    ok &= (prefs.putFloat("batt_low",    battLow)          > 0);
    ok &= (prefs.putFloat("batt_hyst",   battHysteresis)   > 0);

    // Temperature
    ok &= (prefs.putFloat("temp_tgt",    tempTarget)       > 0);
    ok &= (prefs.putFloat("temp_hyst",   tempHysteresis)   > 0);

    // Current
    ok &= (prefs.putFloat("curr_oc",     currentOvercurrent) > 0);
    ok &= (prefs.putFloat("curr_rev",    currentReverse)     > 0);
    ok &= (prefs.putFloat("curr_hyst",   currentHysteresis)  > 0);

    prefs.end();

    if (ok) {
        ESP_LOGI(TAG, "Settings saved to NVS");
    } else {
        ESP_LOGE(TAG, "Some settings failed to save");
    }
    return ok;
}

void ConfigManager::resetToDefaults() {
    loadDefaults();
    save();
    ESP_LOGI(TAG, "Settings reset to defaults");
}

void ConfigManager::applyTo(MoaBattControl& batt, MoaCurrentControl& current,
                            MoaTempControl& temp, ESCController& esc) {
    // Battery configuration (medium = zone between high and low)
    batt.setDividerRatio(BATT_DIVIDER_RATIO);
    batt.setHighThreshold(battHigh);
    batt.setLowThreshold(battLow);
    batt.setHysteresis(battHysteresis);

    // Current sensor configuration
    current.setSensitivity(CURRENT_SENSOR_SENSITIVITY);
    current.setZeroOffset(CURRENT_SENSOR_OFFSET);
    current.setOvercurrentThreshold(currentOvercurrent);
    current.setReverseOvercurrentThreshold(currentReverse);
    current.setHysteresis(currentHysteresis);

    // Temperature configuration
    temp.setTargetTemp(tempTarget);
    temp.setHysteresis(tempHysteresis);

    // ESC configuration
    esc.setRampRate(escRampRate);

    ESP_LOGI(TAG, "Configuration applied to devices");
    ESP_LOGD(TAG, "  Batt: high=%.2fV, med=%.2fV, low=%.2fV, hyst=%.2fV", battHigh, battMedium, battLow, battHysteresis);
    ESP_LOGD(TAG, "  Current: OC=%.1fA, rev=%.1fA, hyst=%.1fA", currentOvercurrent, currentReverse, currentHysteresis);
    ESP_LOGD(TAG, "  Temp: target=%.1fC, hyst=%.1fC", tempTarget, tempHysteresis);
    ESP_LOGD(TAG, "  ESC ramp: %.1f%%/s", escRampRate);
}

uint8_t ConfigManager::throttleLevel(uint8_t commandType) const {
    switch (commandType) {
        case COMMAND_BUTTON_25:  return escEcoMode;
        case COMMAND_BUTTON_50:  return escPaddleMode;
        case COMMAND_BUTTON_75:  return escBreakingMode;
        case COMMAND_BUTTON_100: return escFullThrottle;
        default: return 0;
    }
}

uint32_t ConfigManager::throttleTimeout(uint8_t commandType) const {
    switch (commandType) {
        case COMMAND_BUTTON_25:  return escTime25;
        case COMMAND_BUTTON_50:  return escTime50;
        case COMMAND_BUTTON_75:  return escTime75;
        case COMMAND_BUTTON_100: return escTime100;
        default: return 0;
    }
}
