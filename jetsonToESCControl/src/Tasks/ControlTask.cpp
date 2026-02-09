/**
 * @file ControlTask.cpp
 * @brief FreeRTOS task for event processing and state machine
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "Tasks.h"
#include "MoaMainUnit.h"
#include "esp_log.h"

static const char* TAG = "ControlTask";

void ControlTask(void* pvParameters) {
    MoaMainUnit* unit = static_cast<MoaMainUnit*>(pvParameters);
    ControlCommand cmd;
    
    ESP_LOGI(TAG, "ControlTask started");
    
    for (;;) {
        // Block until an event arrives in the queue
        if (xQueueReceive(unit->getEventQueue(), &cmd, portMAX_DELAY) == pdTRUE) {
            ESP_LOGD(TAG, "Event received: controlType=%d, commandType=%d, value=%d", cmd.controlType, cmd.commandType, cmd.value);
            // Route event to state machine manager
            unit->getStateMachineManager().handleEvent(cmd);
        }
    }
}
