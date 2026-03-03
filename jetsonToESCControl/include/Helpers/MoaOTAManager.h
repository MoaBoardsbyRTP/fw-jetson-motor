/**
 * @file MoaOTAManager.h
 * @brief WiFi STA + ArduinoOTA manager for wireless firmware updates
 * @author Oscar Martinez
 * @date 2026-02-27
 * 
 * Connects to an existing WiFi router (STA mode) and enables ArduinoOTA
 * so the board can be flashed wirelessly. WiFi credentials are read from
 * ConfigManager (NVS-backed, CLI-configurable).
 * Designed to run in its own FreeRTOS task.
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "esp_log.h"

class ConfigManager;

/**
 * @brief WiFi connection timeout in milliseconds
 */
#define OTA_WIFI_CONNECT_TIMEOUT_MS 15000

/**
 * @brief Manages WiFi STA connection and ArduinoOTA
 * 
 * Connects to a router using credentials from ConfigManager and enables
 * ArduinoOTA for wireless firmware updates.
 * 
 * ## Usage
 * @code
 * MoaOTAManager ota(config);
 * ota.begin();   // connects to WiFi + starts OTA
 * // In a task loop:
 * ota.handle();
 * @endcode
 */
class MoaOTAManager {
public:
    /**
     * @brief Construct OTA manager
     * @param config Reference to ConfigManager for WiFi credentials
     */
    explicit MoaOTAManager(ConfigManager& config);
    ~MoaOTAManager() = default;

    /**
     * @brief Connect to WiFi and start ArduinoOTA
     * @return true if WiFi connected and OTA initialized
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
     * @brief Check if WiFi is connected
     */
    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }

    /**
     * @brief Disconnect WiFi and stop ArduinoOTA
     */
    void stop();

    /**
     * @brief Check if OTA manager is active (WiFi connected + OTA running)
     */
    bool isActive() const { return _active; }

private:
    ConfigManager& _config;
    bool _updating;
    bool _active;

    bool connectWiFi();
    void setupOTA();
};
