/**
 * @file Tasks.h
 * @brief FreeRTOS task function declarations
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * Declares the FreeRTOS task functions. Tasks are thin wrappers that
 * call update() on the appropriate producers/consumers.
 */

#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Constants.h"

/**
 * @brief Sensor monitoring task
 * 
 * Periodically calls update() on temperature, battery, and current
 * sensor producers. Runs at TASK_PERIOD_SENSOR_MS interval.
 * 
 * @param pvParameters Pointer to MoaMainUnit instance
 */
void SensorTask(void* pvParameters);

/**
 * @brief I/O handling task
 * 
 * Periodically calls update() on button input and LED output.
 * Runs at TASK_PERIOD_IO_MS interval.
 * 
 * @param pvParameters Pointer to MoaMainUnit instance
 */
void IOTask(void* pvParameters);

/**
 * @brief Control task (event-driven)
 * 
 * Blocks on event queue, routes events to state machine manager.
 * Also handles periodic flash log updates.
 * 
 * @param pvParameters Pointer to MoaMainUnit instance
 */
void ControlTask(void* pvParameters);
