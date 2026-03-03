/**
 * @file MoaWiFiManager.cpp
 * @brief Implementation of the MoaWiFiManager class
 * @author Oscar Martinez
 * @date 2026-02-28
 */

#include "MoaWiFiManager.h"
#include "Constants.h"

static const char* TAG = "WiFiManager";

MoaWiFiManager::MoaWiFiManager(const char* apSsid, const char* apPassword)
    : _hasPassword(false)
    , _running(false) {
    setCredentials(apSsid, apPassword);
}

void MoaWiFiManager::setCredentials(const char* apSsid, const char* apPassword) {
    if (apSsid == nullptr || apSsid[0] == '\0') {
        strncpy(_apSsid, OTA_AP_SSID, sizeof(_apSsid) - 1);
    } else {
        strncpy(_apSsid, apSsid, sizeof(_apSsid) - 1);
    }
    _apSsid[sizeof(_apSsid) - 1] = '\0';

    if (apPassword == nullptr || apPassword[0] == '\0') {
        _apPassword[0] = '\0';
        _hasPassword = false;
    } else {
        strncpy(_apPassword, apPassword, sizeof(_apPassword) - 1);
        _apPassword[sizeof(_apPassword) - 1] = '\0';
        _hasPassword = true;
    }

    if (_running) {
        ESP_LOGW(TAG, "Credentials updated while AP running; changes apply after restart");
    }
}

bool MoaWiFiManager::start() {
    if (_running) {
        ESP_LOGW(TAG, "WiFi AP already running");
        return true;
    }

    ESP_LOGI(TAG, "Starting WiFi AP...");

    if (!startAP()) {
        ESP_LOGE(TAG, "Failed to start WiFi AP");
        return false;
    }

    _running = true;
    return true;
}

void MoaWiFiManager::stop() {
    if (!_running) {
        return;
    }

    ESP_LOGI(TAG, "Stopping WiFi AP...");

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    _running = false;

    ESP_LOGI(TAG, "WiFi AP stopped");
}

IPAddress MoaWiFiManager::getIP() const {
    if (!_running) {
        return IPAddress(0, 0, 0, 0);
    }
    return WiFi.softAPIP();
}

int MoaWiFiManager::getStationCount() const {
    if (!_running) {
        return 0;
    }
    return WiFi.softAPgetStationNum();
}

bool MoaWiFiManager::startAP() {
    // Disable STA mode completely - we only want AP mode
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);  // Allow mode change to settle

    // Set low TX power (workaround for ESP32-C3 antenna layout)
    WiFi.setTxPower(WIFI_POWER_8_5dBm);

    // Enable AP mode only
    WiFi.mode(WIFI_AP);

    // Start soft AP
    bool result;
    if (_hasPassword) {
        result = WiFi.softAP(_apSsid, _apPassword);
    } else {
        result = WiFi.softAP(_apSsid);
    }

    if (!result) {
        ESP_LOGE(TAG, "softAP() failed");
        return false;
    }

    // Wait for AP to be ready and get IP
    int retries = 50;  // 5 seconds max
    IPAddress ip;
    while ((ip = WiFi.softAPIP()) == IPAddress(0, 0, 0, 0) && retries > 0) {
        delay(100);
        retries--;
    }

    if (ip == IPAddress(0, 0, 0, 0)) {
        ESP_LOGE(TAG, "AP failed to get IP address");
        return false;
    }

    ESP_LOGI(TAG, "AP started: SSID=%s  IP=%s  Stations=%d",
             _apSsid, ip.toString().c_str(), WiFi.softAPgetStationNum());

    return true;
}
