/**
 * @file SensorTask.cpp
 * @brief FreeRTOS task for sensor monitoring
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "Tasks.h"
#include "MoaMainUnit.h"

void SensorTask(void* pvParameters) {
    MoaMainUnit* unit = static_cast<MoaMainUnit*>(pvParameters);
    
    for (;;) {
        // Update all sensor producers
        // Each will push events to the queue if thresholds are crossed
        unit->getTempControl().update();
        unit->getBattControl().update();
        unit->getCurrentControl().update();
        
        vTaskDelay(pdMS_TO_TICKS(TASK_SENSOR_PERIOD_MS));
    }
}
