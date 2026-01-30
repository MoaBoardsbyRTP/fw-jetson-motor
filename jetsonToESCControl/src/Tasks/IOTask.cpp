/**
 * @file IOTask.cpp
 * @brief FreeRTOS task for I/O handling (buttons and LEDs)
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "Tasks.h"
#include "MoaMainUnit.h"

void IOTask(void* pvParameters) {
    MoaMainUnit* unit = static_cast<MoaMainUnit*>(pvParameters);
    
    for (;;) {
        // Update button input (pushes events on press/long-press)
        unit->getButtonControl().update();
        
        // Update LED output (drives blink timing)
        unit->getLedControl().update();
        
        vTaskDelay(pdMS_TO_TICKS(TASK_IO_PERIOD_MS));
    }
}
