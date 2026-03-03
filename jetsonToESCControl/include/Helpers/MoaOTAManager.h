/**
 * @file MoaOTAManager.h
 * @brief ArduinoOTA manager using shared WiFi manager
 * @author Oscar Martinez
 * @date 2026-02-28
 * 
 * Uses MoaWiFiManager for WiFi lifecycle.
 */

#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "esp_log.h"
#include "MoaWiFiManager.h"

/**
 * @brief Manages ArduinoOTA using shared WiFi manager
 */
class MoaOTAManager {
public:
    /**
     * @brief Construct OTA manager
     * @param wifiManager Reference to shared WiFi manager
     * @param hostname Optional mDNS hostname for OTA discovery
     */
    MoaOTAManager(MoaWiFiManager& wifiManager, const char* hostname = nullptr);

    /**
     * @brief Set OTA mDNS hostname
     */
    void setHostname(const char* hostname);

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
    MoaWiFiManager& _wifiManager;
    const char* _hostname;
    bool _updating;
    bool _active;
    void setupOTA();
};
