/**
 * @file MoaWiFiManager.h
 * @brief WiFi STA manager shared by OTA and WebServer
 * @author Oscar Martinez
 * @date 2026-02-28
 * 
 * Manages WiFi STA lifecycle independently of OTA.
 * Can be shared between OTA, webserver, and future services.
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "esp_log.h"

/**
 * @brief Manages WiFi STA lifecycle
 * 
 * Connects to a router using configured credentials and provides
 * a shared WiFi connection that can be used by multiple services
 * (OTA, webserver, etc.). Handles the low TX power workaround for
 * the ESP32-C3 antenna layout.
 */
class MoaWiFiManager {
public:
    /**
     * @brief Construct WiFi manager
     * @param apSsid     Router SSID
     * @param apPassword Router password (nullptr or empty for open network)
     */
    MoaWiFiManager(const char* apSsid, const char* apPassword = nullptr);
    ~MoaWiFiManager() = default;

    /**
     * @brief Update router credentials
     * @param apSsid Router SSID
     * @param apPassword Router password (nullptr or empty for open network)
     */
    void setCredentials(const char* apSsid, const char* apPassword = nullptr);

    /**
     * @brief Connect WiFi STA to configured router
     * @return true if connected successfully
     */
    bool start();

    /**
     * @brief Disconnect WiFi STA
     */
    void stop();

    /**
     * @brief Check if STA connection is active
     */
    bool isRunning() const { return _running; }

    /**
     * @brief Get STA local IP address
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

    bool connectSTA();
};
