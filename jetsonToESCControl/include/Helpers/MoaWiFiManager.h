/**
 * @file MoaWiFiManager.h
 * @brief WiFi AP manager shared by OTA and WebServer
 * @author Oscar Martinez
 * @date 2026-02-28
 * 
 * Manages WiFi Soft AP lifecycle independently of OTA.
 * Can be shared between OTA, webserver, and future services.
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "esp_log.h"

/**
 * @brief Manages WiFi Soft AP lifecycle
 * 
 * Provides a shared WiFi AP that can be used by multiple services
 * (OTA, webserver, etc.). Handles the low TX power workaround for
 * the ESP32-C3 antenna layout.
 */
class MoaWiFiManager {
public:
    /**
     * @brief Construct WiFi manager
     * @param apSsid     Soft AP SSID
     * @param apPassword Soft AP password (min 8 chars, nullptr for open)
     */
    MoaWiFiManager(const char* apSsid, const char* apPassword = nullptr);
    ~MoaWiFiManager() = default;

    /**
     * @brief Update AP credentials
     * @param apSsid Soft AP SSID
     * @param apPassword Soft AP password (nullptr or empty for open AP)
     */
    void setCredentials(const char* apSsid, const char* apPassword = nullptr);

    /**
     * @brief Start WiFi AP
     * @return true if AP started successfully
     */
    bool start();

    /**
     * @brief Stop WiFi AP
     */
    void stop();

    /**
     * @brief Check if AP is running
     */
    bool isRunning() const { return _running; }

    /**
     * @brief Get AP IP address
     * @return IP address (0.0.0.0 if not running)
     */
    IPAddress getIP() const;

    /**
     * @brief Get connected station count
     */
    int getStationCount() const;

private:
    char _apSsid[33];
    char _apPassword[65];
    bool _hasPassword;
    bool _running;

    bool startAP();
};
