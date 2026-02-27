/**
 * @file MoaOTAManager.cpp
 * @brief Implementation of the MoaOTAManager class
 * @author Oscar Martinez
 * @date 2026-02-27
 */

#include "MoaOTAManager.h"

static const char* TAG = "OTAManager";

MoaOTAManager::MoaOTAManager(const char* apSsid, const char* apPassword,
                             const char* hostname)
    : _apSsid(apSsid)
    , _apPassword(apPassword)
    , _hostname(hostname)
    , _apActive(false)
    , _updating(false)
    , _active(false) {}

bool MoaOTAManager::begin() {
    ESP_LOGI(TAG, "Initializing WiFi AP + OTA...");

    if (!startAP()) {
        ESP_LOGE(TAG, "Failed to start WiFi AP");
        return false;
    }

    setupOTA();

    ArduinoOTA.begin();
    _active = true;
    ESP_LOGI(TAG, "ArduinoOTA started on %s", WiFi.localIP().toString().c_str());

    return true;
}

void MoaOTAManager::handle() {
    if (_active) {
        ArduinoOTA.handle();
    }
}

void MoaOTAManager::stop() {
    if (_active) {
        ArduinoOTA.end();
        //WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        _active = false;
        _apActive = false;
        ESP_LOGI(TAG, "WiFi AP stopped");
    }
}

bool MoaOTAManager::startAP() {
    // Use AP mode only — don't touch STA so it won't interfere with anything
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.mode(WIFI_AP);
    bool result;
    if (_apPassword)
        result = WiFi.begin(_apSsid, _apPassword);
    if (!result) {
        ESP_LOGE(TAG, "WiFi.begin() failed");
        return false;
    }

    _apActive = true;
    ESP_LOGI(TAG, "AP started: SSID=%s  IP=%s",
             _apSsid, WiFi.localIP().toString().c_str());

    return true;
}

void MoaOTAManager::setupOTA() {
    if (_hostname) {
        ArduinoOTA.setHostname(_hostname);
        ESP_LOGI(TAG, "OTA hostname: %s", _hostname);
    }

    ArduinoOTA.onStart([this]() {
        _updating = true;
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        ESP_LOGI(TAG, "OTA start: updating %s", type.c_str());
    });

    ArduinoOTA.onEnd([this]() {
        _updating = false;
        ESP_LOGI(TAG, "OTA complete — rebooting...");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static unsigned int lastPercent = 0;
        unsigned int percent = (progress * 100) / total;
        if (percent != lastPercent && percent % 10 == 0) {
            ESP_LOGI("OTA", "Progress: %u%%", percent);
            lastPercent = percent;
        }
    });

    ArduinoOTA.onError([this](ota_error_t error) {
        _updating = false;
        const char* errStr;
        switch (error) {
            case OTA_AUTH_ERROR:    errStr = "Auth failed"; break;
            case OTA_BEGIN_ERROR:   errStr = "Begin failed"; break;
            case OTA_CONNECT_ERROR: errStr = "Connect failed"; break;
            case OTA_RECEIVE_ERROR: errStr = "Receive failed"; break;
            case OTA_END_ERROR:     errStr = "End failed"; break;
            default:                errStr = "Unknown"; break;
        }
        ESP_LOGE(TAG, "OTA error[%u]: %s", error, errStr);
    });
}
