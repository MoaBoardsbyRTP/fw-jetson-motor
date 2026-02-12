/**
 * @file StatsTask.cpp
 * @brief FreeRTOS task for stats aggregation
 * @author Oscar Martinez
 * @date 2025-02-03
 */

#include "Tasks.h"
#include "MoaMainUnit.h"
#include "StatsReading.h"
#include "esp_log.h"

static const char* TAG = "StatsTask";

void StatsTask(void* pvParameters) {
    MoaMainUnit* unit = static_cast<MoaMainUnit*>(pvParameters);
    StatsReading reading;
    
    ESP_LOGI(TAG, "StatsTask started");
    
    for (;;) {
        // Block until a stats reading arrives in the queue
        if (xQueueReceive(unit->getStatsQueue(), &reading, portMAX_DELAY) == pdTRUE) {
            ESP_LOGV(TAG, "Stats reading: type=%d, value=%ld, ts=%lu", reading.statsType, reading.value, reading.timestamp);
            // Update the stats aggregator
            unit->getStatsAggregator().update(reading);
        }
    }
}
