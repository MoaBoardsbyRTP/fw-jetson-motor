/**
 * @file MoaOTAManager.h
 * @brief WiFi AP + ArduinoOTA manager for wireless firmware updates
 * @author Oscar Martinez
 * @date 2026-02-27
 * 
 * Starts a WiFi Soft AP and enables ArduinoOTA so the board can be
 * flashed wirelessly. Designed to run in its own FreeRTOS task.
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "esp_log.h"

/**
 * @brief Manages WiFi AP mode and ArduinoOTA
 * 
 * Creates a Soft AP with configurable SSID/password so a developer
 * can connect and push firmware updates over the air.
 * 
 * ## Usage
 * @code
 * MoaOTAManager ota("MoaESC", "moa12345");
 * ota.begin();
 * // In a task loop:
 * ota.handle();
 * @endcode
 */
class MoaOTAManager {
public:
    /**
     * @brief Construct OTA manager
     * @param apSsid     Soft AP SSID
     * @param apPassword Soft AP password (min 8 chars, nullptr for open)
     * @param hostname   mDNS hostname for OTA discovery
     */
    MoaOTAManager(const char* apSsid, const char* apPassword = nullptr,
                  const char* hostname = nullptr);
    ~MoaOTAManager() = default;

    /**
     * @brief Start WiFi AP and ArduinoOTA
     * @return true if both initialized successfully
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
     * @brief Check if WiFi AP is active
     */
    bool isApActive() const { return _apActive; }

    /**
     * @brief Tear down WiFi AP and stop ArduinoOTA
     */
    void stop();

    /**
     * @brief Check if OTA manager is active (AP running)
     */
    bool isActive() const { return _active; }

private:
    const char* _apSsid;
    const char* _apPassword;
    const char* _hostname;
    bool _apActive;
    bool _updating;
    bool _active;

    bool startAP();
    void setupOTA();
};
