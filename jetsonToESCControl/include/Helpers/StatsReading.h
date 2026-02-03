/**
 * @file StatsReading.h
 * @brief Stats reading structure for telemetry queue
 * @author Oscar Martinez
 * @date 2025-02-03
 * 
 * Lightweight structure for continuous sensor readings sent to the
 * stats queue. Separate from ControlCommand to avoid impacting the
 * control event queue.
 */

#pragma once

#include <Arduino.h>

/**
 * @brief Stats type identifiers
 */
#define STATS_TYPE_TEMPERATURE  1
#define STATS_TYPE_BATTERY      2
#define STATS_TYPE_CURRENT      3

/**
 * @brief Stats reading structure for telemetry
 * 
 * Sent by sensors on every update() call to provide continuous
 * readings for telemetry, logging, and monitoring.
 */
struct StatsReading {
    uint8_t statsType;    ///< Type identifier (STATS_TYPE_*)
    int32_t value;        ///< Reading value (scaled as per type)
    uint32_t timestamp;   ///< millis() timestamp of reading
};
