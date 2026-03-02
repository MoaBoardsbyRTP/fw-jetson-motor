/**
 * @file MoaOTAManager.h
 * @brief ArduinoOTA manager using shared WiFi manager
 * @author Oscar Martinez
 * @date 2026-02-28
 * 
 * Uses MoaWiFiManager for WiFi lifecycle. WiFi credentials and OTA
 * hostname are provided by ConfigManager (NVS-backed, CLI-configurable).
 */

#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "esp_log.h"
#include "MoaWiFiManager.h"

class ConfigManager;

/**
 * @brief Manages ArduinoOTA using shared WiFi manager
 */
class MoaOTAManager {
public:
    /**
     * @brief Construct OTA manager
     * @param config Reference to ConfigManager for WiFi credentials
     * @param wifiManager Reference to shared WiFi manager
     */
    MoaOTAManager(ConfigManager& config, MoaWiFiManager& wifiManager);

    /**
     * @brief Start ArduinoOTA (WiFi must be started separately)
     * @return true if OTA initialized successfully
     */
    bool begin();

    /**
     * @brief Handle OTA requests (call frequently in task loop)
     */
    void handle();

    /**
     * @brief Check if an OTA update is in progress
     */
    bool isUpdating() const { return _updating; }

    /**
     * @brief Check if OTA is active
     */
    bool isActive() const { return _active; }

    /**
     * @brief Stop ArduinoOTA
     */
    void stop();

private:
    ConfigManager& _config;
    MoaWiFiManager& _wifiManager;
    bool _updating;
    bool _active;
    void setupOTA();
};
