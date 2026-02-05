/**
 * @file IOTask.cpp
 * @brief FreeRTOS task for I/O handling (buttons and LEDs)
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * Button handling is interrupt-driven:
 * - ISR attached to MCP23018 INTA pin triggers on button change
 * - IOTask processes interrupt via processInterrupt()
 * - Debounce handled in processInterrupt() using INTCAPA register
 * - Long-press detection checked periodically in this task
 */

#include "Tasks.h"
#include "MoaMainUnit.h"

void IOTask(void* pvParameters) {
    MoaMainUnit* unit = static_cast<MoaMainUnit*>(pvParameters);
    
    for (;;) {
        // Process button interrupt if pending
        // This reads INTCAPA, handles debounce, and clears MCP interrupt
        if (unit->getButtonControl().isInterruptPending()) {
            unit->getButtonControl().processInterrupt();
        }
        
        // Check for long-press events (must be polled)
        // This updates button hold times and fires long-press events
        unit->getButtonControl().checkLongPress();
        
        // Update LED output (drives blink timing)
        unit->getLedControl().update();
        
        vTaskDelay(pdMS_TO_TICKS(TASK_IO_PERIOD_MS));
    }
}
