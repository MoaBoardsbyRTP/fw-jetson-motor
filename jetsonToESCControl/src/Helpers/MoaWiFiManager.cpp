/**
 * @file MoaWiFiManager.cpp
 * @brief Implementation of the MoaWiFiManager class
 * @author Oscar Martinez
 * @date 2026-02-28
 */

#include "MoaWiFiManager.h"
#include "Constants.h"

static const char* TAG = "WiFiManager";
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 25000;
static constexpr uint32_t WIFI_CONNECT_POLL_MS = 250;

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
        ESP_LOGW(TAG, "WiFi STA already connected");
        return true;
    }

    ESP_LOGI(TAG, "Starting WiFi STA connection...");

    if (!connectSTA()) {
        ESP_LOGE(TAG, "Failed to connect WiFi STA");
        return false;
    }

    _running = true;
    return true;
}

void MoaWiFiManager::stop() {
    if (!_running) {
        return;
    }

    ESP_LOGI(TAG, "Disconnecting WiFi STA...");

    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    _running = false;

    ESP_LOGI(TAG, "WiFi STA disconnected");
}

IPAddress MoaWiFiManager::getIP() const {
    if (!_running) {
        return IPAddress(0, 0, 0, 0);
    }
    return WiFi.localIP();
}

int MoaWiFiManager::getStationCount() const {
    if (!_running) {
        return 0;
    }
    return (WiFi.status() == WL_CONNECTED) ? 1 : 0;
}

bool MoaWiFiManager::connectSTA() {
    if (_apSsid[0] == '\0') {
        ESP_LOGE(TAG, "WiFi SSID empty, cannot connect");
        return false;
    }

    // === Clean radio state (mirrors TestWifi disconnect pattern) ===
    ESP_LOGD(TAG, "Resetting WiFi radio state (status=%d)", static_cast<int>(WiFi.status()));
    WiFi.disconnect(true, true);   // disconnect + erase stored AP credentials
    delay(100);

    // Wait for idle — without this, begin() fires on a dirty stack
    int idleWait = 0;
    while (WiFi.status() != WL_IDLE_STATUS && idleWait < 20) {
        delay(50);
        idleWait++;
    }
    ESP_LOGD(TAG, "After reset: status=%d (idle_waits=%d)", static_cast<int>(WiFi.status()), idleWait);

    // If still not idle, force a full mode cycle
    if (WiFi.status() != WL_IDLE_STATUS) {
        ESP_LOGW(TAG, "Radio not idle, forcing WIFI_OFF → WIFI_STA cycle");
        WiFi.mode(WIFI_OFF);
        delay(100);
        WiFi.mode(WIFI_STA);
        delay(100);
    }

    // Settle delay before scan/connect
    delay(200);

    // Set low TX power (workaround for ESP32-C3 antenna layout)
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    ESP_LOGD(TAG, "TX power set to WIFI_POWER_8_5dBm");

    // Pre-scan to cache the AP's channel — without this begin() does a blind
    // probe across all channels and gets no response (mirrors TestWifi flow)
    ESP_LOGD(TAG, "Pre-scanning to cache AP channel info...");
    int found = WiFi.scanNetworks(false, false);
    ESP_LOGD(TAG, "Pre-scan done: %d networks found", found);

    if (_hasPassword) {
        WiFi.begin(_apSsid, _apPassword);
        ESP_LOGI(TAG, "Connecting to router SSID='%s'", _apSsid);
    } else {
        WiFi.begin(_apSsid);
        ESP_LOGI(TAG, "Connecting to router SSID='%s' (open)", _apSsid);
    }

    uint32_t startMs = millis();
    uint32_t dots = 0;
    while (WiFi.status() != WL_CONNECTED) {
        uint32_t elapsed = millis() - startMs;
        if (elapsed >= WIFI_CONNECT_TIMEOUT_MS) {
            ESP_LOGE(TAG, "WiFi connect timeout after %lu ms (status=%d)",
                     static_cast<unsigned long>(elapsed), static_cast<int>(WiFi.status()));
            WiFi.disconnect(true, true);
            WiFi.mode(WIFI_OFF);
            return false;
        }

        if ((dots++ % 4U) == 0U) {
            ESP_LOGD(TAG, "Waiting for WiFi... elapsed=%lu ms status=%d",
                     static_cast<unsigned long>(elapsed), static_cast<int>(WiFi.status()));
        }
        vTaskDelay(pdMS_TO_TICKS(WIFI_CONNECT_POLL_MS));
    }

    IPAddress ip = WiFi.localIP();
    if (ip == IPAddress(0, 0, 0, 0)) {
        ESP_LOGE(TAG, "WiFi connected but local IP is invalid");
        return false;
    }

    ESP_LOGI(TAG, "WiFi connected: SSID=%s IP=%s RSSI=%d",
             _apSsid, ip.toString().c_str(), WiFi.RSSI());

    return true;
}
