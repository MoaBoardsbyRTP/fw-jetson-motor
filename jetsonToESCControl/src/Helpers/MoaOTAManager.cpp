/**
 * @file MoaOTAManager.cpp
 * @brief Implementation of the MoaOTAManager class
 * @author Oscar Martinez
 * @date 2026-02-28
 */

#include "MoaOTAManager.h"
#include "ConfigManager.h"

static const char* TAG = "OTAManager";

MoaOTAManager::MoaOTAManager(ConfigManager& config, MoaWiFiManager& wifiManager)
    : _config(config)
    , _wifiManager(wifiManager)
    , _updating(false)
    , _active(false) {}

bool MoaOTAManager::begin() {
    if (_active) {
        ESP_LOGW(TAG, "OTA already active");
        return true;
    }

    ESP_LOGI(TAG, "Initializing ArduinoOTA...");

    // WiFi must be started separately by MoaWiFiManager
    if (!_wifiManager.isRunning()) {
        ESP_LOGE(TAG, "WiFi not running - start MoaWiFiManager first");
        return false;
    }

    setupOTA();
    ArduinoOTA.begin();
    _active = true;

    ESP_LOGI(TAG, "ArduinoOTA started on %s", _wifiManager.getIP().toString().c_str());
    return true;
}

void MoaOTAManager::handle() {
    if (_active) {
        ArduinoOTA.handle();
    }
}

void MoaOTAManager::stop() {
    if (!_active) {
        return;
    }

    ESP_LOGI(TAG, "Stopping ArduinoOTA...");
    ArduinoOTA.end();
    _active = false;
    _updating = false;
}

void MoaOTAManager::setupOTA() {
    const char* hostname = _config.otaHostname;
    if (hostname[0] != '\0') {
        ArduinoOTA.setHostname(hostname);
        ESP_LOGI(TAG, "OTA hostname: %s", hostname);
    }

    ArduinoOTA.onStart([this]() {
        _updating = true;
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        ESP_LOGI(TAG, "OTA start: updating %s", type.c_str());
    });

    ArduinoOTA.onEnd([this]() {
        _updating = false;
        ESP_LOGI(TAG, "OTA complete - rebooting...");
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
