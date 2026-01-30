/**
 * @file main.cpp
 * @brief Moa ESC Controller entry point
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * Ultra-clean main file. All initialization and task management
 * is handled by MoaMainUnit.
 */

#include <Arduino.h>
#include "MoaMainUnit.h"

MoaMainUnit mainUnit;

void setup() {
    mainUnit.begin();
}

void loop() {
    // Empty - FreeRTOS tasks handle everything
}