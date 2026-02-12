/**
 * @file MoaStatsAggregator.cpp
 * @brief Implementation of the MoaStatsAggregator class
 * @author Oscar Martinez
 * @date 2025-02-03
 */

#include "MoaStatsAggregator.h"
#include "esp_log.h"

static const char* TAG = "Stats";

MoaStatsAggregator::MoaStatsAggregator()
    : _mutex(nullptr)
{
    _stats.temperatureX10 = 0;
    _stats.batteryVoltageMv = 0;
    _stats.currentX10 = 0;
    _stats.tempTimestamp = 0;
    _stats.battTimestamp = 0;
    _stats.currentTimestamp = 0;
}

MoaStatsAggregator::~MoaStatsAggregator() {
    if (_mutex != nullptr) {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
}

void MoaStatsAggregator::begin() {
    _mutex = xSemaphoreCreateMutex();
    ESP_LOGD(TAG, "Stats aggregator initialized");
}

void MoaStatsAggregator::update(const StatsReading& reading) {
    if (_mutex == nullptr) {
        return;
    }

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        switch (reading.statsType) {
            case STATS_TYPE_TEMPERATURE:
                _stats.temperatureX10 = static_cast<int16_t>(reading.value);
                _stats.tempTimestamp = reading.timestamp;
                break;

            case STATS_TYPE_BATTERY:
                _stats.batteryVoltageMv = static_cast<int16_t>(reading.value);
                _stats.battTimestamp = reading.timestamp;
                break;

            case STATS_TYPE_CURRENT:
                _stats.currentX10 = static_cast<int16_t>(reading.value);
                _stats.currentTimestamp = reading.timestamp;
                break;

            default:
                break;
        }
        xSemaphoreGive(_mutex);
    }
}

StatsSnapshot MoaStatsAggregator::getSnapshot() {
    StatsSnapshot snapshot = {};

    if (_mutex != nullptr && xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        snapshot = _stats;
        xSemaphoreGive(_mutex);
    }

    return snapshot;
}

int16_t MoaStatsAggregator::getTemperatureX10() {
    int16_t value = 0;

    if (_mutex != nullptr && xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        value = _stats.temperatureX10;
        xSemaphoreGive(_mutex);
    }

    return value;
}

int16_t MoaStatsAggregator::getBatteryVoltageMv() {
    int16_t value = 0;

    if (_mutex != nullptr && xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        value = _stats.batteryVoltageMv;
        xSemaphoreGive(_mutex);
    }

    return value;
}

int16_t MoaStatsAggregator::getCurrentX10() {
    int16_t value = 0;

    if (_mutex != nullptr && xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        value = _stats.currentX10;
        xSemaphoreGive(_mutex);
    }

    return value;
}
