/**
 * @file MoaStatsAggregator.h
 * @brief Centralized stats storage for telemetry and monitoring
 * @author Oscar Martinez
 * @date 2025-02-03
 * 
 * MoaStatsAggregator provides a single source of truth for current device
 * readings. It consumes StatsReading messages from the stats queue and
 * provides thread-safe access to the latest values.
 */

#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "StatsReading.h"

/**
 * @brief Snapshot of all current stats
 * 
 * Returned by getSnapshot() for atomic access to all readings.
 */
struct StatsSnapshot {
    int16_t temperatureX10;     ///< Temperature in °C × 10 (e.g., 255 = 25.5°C)
    int16_t batteryVoltageMv;   ///< Battery voltage in millivolts
    int16_t currentX10;         ///< Current in A × 10 (e.g., 1255 = 125.5A)
    uint32_t tempTimestamp;     ///< Last temperature update (millis)
    uint32_t battTimestamp;     ///< Last battery update (millis)
    uint32_t currentTimestamp;  ///< Last current update (millis)
};

/**
 * @brief Centralized stats aggregator for telemetry
 * 
 * Stores the latest sensor readings and provides thread-safe access
 * for telemetry consumers (webserver, serial logging, etc.).
 * 
 * ## Usage
 * @code
 * MoaStatsAggregator stats;
 * stats.begin();
 * 
 * // In StatsTask:
 * StatsReading reading;
 * if (xQueueReceive(statsQueue, &reading, portMAX_DELAY)) {
 *     stats.update(reading);
 * }
 * 
 * // In telemetry consumer:
 * StatsSnapshot snapshot = stats.getSnapshot();
 * float temp = snapshot.temperatureX10 / 10.0f;
 * @endcode
 */
class MoaStatsAggregator {
public:
    /**
     * @brief Construct a new MoaStatsAggregator
     */
    MoaStatsAggregator();

    /**
     * @brief Destructor
     */
    ~MoaStatsAggregator();

    /**
     * @brief Initialize the aggregator (creates mutex)
     */
    void begin();

    /**
     * @brief Update stats with a new reading
     * 
     * Called by StatsTask when a reading arrives from the queue.
     * Thread-safe.
     * 
     * @param reading The stats reading to process
     */
    void update(const StatsReading& reading);

    /**
     * @brief Get a snapshot of all current stats
     * 
     * Returns an atomic copy of all readings. Thread-safe.
     * 
     * @return StatsSnapshot Current stats values
     */
    StatsSnapshot getSnapshot();

    /**
     * @brief Get current temperature (×10)
     * @return int16_t Temperature in °C × 10
     */
    int16_t getTemperatureX10();

    /**
     * @brief Get current battery voltage in millivolts
     * @return int16_t Voltage in mV
     */
    int16_t getBatteryVoltageMv();

    /**
     * @brief Get current current reading (×10)
     * @return int16_t Current in A × 10
     */
    int16_t getCurrentX10();

private:
    SemaphoreHandle_t _mutex;
    StatsSnapshot _stats;
};
