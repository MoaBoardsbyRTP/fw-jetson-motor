/**
 * @file StatsTask.cpp
 * @brief FreeRTOS task for stats aggregation
 * @author Oscar Martinez
 * @date 2025-02-03
 */

#include "Tasks.h"
#include "MoaMainUnit.h"
#include "StatsReading.h"

void StatsTask(void* pvParameters) {
    MoaMainUnit* unit = static_cast<MoaMainUnit*>(pvParameters);
    StatsReading reading;
    
    for (;;) {
        // Block until a stats reading arrives in the queue
        if (xQueueReceive(unit->getStatsQueue(), &reading, portMAX_DELAY) == pdTRUE) {
            // Update the stats aggregator
            unit->getStatsAggregator().update(reading);
        }
    }
}
