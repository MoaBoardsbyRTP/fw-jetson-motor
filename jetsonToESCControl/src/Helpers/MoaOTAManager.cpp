/**
 * @file MoaOTAManager.cpp
 * @brief Implementation of the MoaOTAManager class
 * @author Oscar Martinez
 * @date 2026-02-27
 */

#include "MoaOTAManager.h"
#include "ConfigManager.h"

static const char* TAG = "OTAManager";

MoaOTAManager::MoaOTAManager(ConfigManager& config)
    : _config(config)
    , _updating(false)
    , _active(false) {}

bool MoaOTAManager::begin() {
    ESP_LOGI(TAG, "Initializing WiFi STA + OTA...");

    if (!connectWiFi()) {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
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
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        _active = false;
        ESP_LOGI(TAG, "WiFi disconnected, OTA stopped");
    }
}

bool MoaOTAManager::connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);

    const char* ssid = _config.wifiSsid;
    const char* pass = _config.wifiPassword;

    if (ssid[0] == '\0') {
        ESP_LOGE(TAG, "WiFi SSID is empty — set via CLI: set wifi_ssid <ssid>");
        return false;
    }

    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", ssid);

    if (pass[0] != '\0') {
        WiFi.begin(ssid, pass);
    } else {
        WiFi.begin(ssid);
    }

    uint32_t startMs = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if ((millis() - startMs) >= OTA_WIFI_CONNECT_TIMEOUT_MS) {
            ESP_LOGE(TAG, "WiFi connection timed out after %ums", OTA_WIFI_CONNECT_TIMEOUT_MS);
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }

    ESP_LOGI(TAG, "WiFi connected: SSID=%s  IP=%s  RSSI=%d",
             ssid, WiFi.localIP().toString().c_str(), WiFi.RSSI());

    return true;
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
