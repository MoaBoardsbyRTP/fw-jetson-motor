/**
 * @file ControlTask.cpp
 * @brief FreeRTOS task for event processing and state machine
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "Tasks.h"
#include "MoaMainUnit.h"

void ControlTask(void* pvParameters) {
    MoaMainUnit* unit = static_cast<MoaMainUnit*>(pvParameters);
    ControlCommand cmd;
    
    for (;;) {
        // Block until an event arrives in the queue
        if (xQueueReceive(unit->getEventQueue(), &cmd, portMAX_DELAY) == pdTRUE) {
            // Route event to state machine manager
            unit->getStateMachineManager().handleEvent(cmd);
        }
    }
}
