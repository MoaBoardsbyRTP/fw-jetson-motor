/**
 * @file MoaMainUnit.h
 * @brief Central coordinator for Moa ESC Controller
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * MoaMainUnit owns all hardware instances and managers, creates the FreeRTOS
 * event queue and tasks. Inspired by RTPBuit's MainUnit pattern but adapted
 * for FreeRTOS-based architecture.
 */

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "PinMapping.h"
#include "Constants.h"
#include "ControlCommand.h"

#include "MoaMcpDevice.h"
#include "MoaTempControl.h"
#include "MoaBattControl.h"
#include "MoaCurrentControl.h"
#include "MoaButtonControl.h"
#include "MoaLedControl.h"
#include "MoaFlashLog.h"
#include "ESCController.h"

#include "MoaDevicesManager.h"
#include "MoaStateMachineManager.h"
#include "MoaStatsAggregator.h"
#include "StatsReading.h"

/**
 * @brief Event queue size (number of ControlCommand items)
 */
#define EVENT_QUEUE_SIZE 16

/**
 * @brief Stats queue size (number of StatsReading items)
 */
#define STATS_QUEUE_SIZE 8

/**
 * @brief Task stack sizes in bytes
 */
#define TASK_STACK_SENSOR   4096
#define TASK_STACK_IO       4096
#define TASK_STACK_CONTROL  4096
#define TASK_STACK_STATS    2048

/**
 * @brief Task priorities (higher = more priority)
 */
#define TASK_PRIORITY_SENSOR    3
#define TASK_PRIORITY_IO        2
#define TASK_PRIORITY_CONTROL   2
#define TASK_PRIORITY_STATS     1

/**
 * @brief Central coordinator for Moa ESC Controller
 * 
 * Owns all hardware instances, managers, and FreeRTOS resources.
 * Provides a clean interface for main.cpp with just begin().
 * 
 * ## Usage
 * @code
 * MoaMainUnit mainUnit;
 * 
 * void setup() {
 *     mainUnit.begin();
 * }
 * 
 * void loop() {
 *     // Empty - FreeRTOS tasks handle everything
 * }
 * @endcode
 */
class MoaMainUnit {
public:
    /**
     * @brief Construct a new MoaMainUnit object
     */
    MoaMainUnit();

    /**
     * @brief Destructor
     */
    ~MoaMainUnit();

    /**
     * @brief Initialize all hardware and start FreeRTOS tasks
     * 
     * This method:
     * 1. Initializes Serial and I2C
     * 2. Creates the event queue
     * 3. Initializes all hardware (sensors, MCP23018, ESC)
     * 4. Applies configuration from Constants.h
     * 5. Creates and starts FreeRTOS tasks
     * 6. Logs system boot
     */
    void begin();

    // === Accessors for FreeRTOS tasks ===

    /**
     * @brief Get the event queue handle
     * @return QueueHandle_t Event queue
     */
    QueueHandle_t getEventQueue();

    /**
     * @brief Get reference to temperature control
     * @return MoaTempControl& Temperature sensor producer
     */
    MoaTempControl& getTempControl();

    /**
     * @brief Get reference to battery control
     * @return MoaBattControl& Battery sensor producer
     */
    MoaBattControl& getBattControl();

    /**
     * @brief Get reference to current control
     * @return MoaCurrentControl& Current sensor producer
     */
    MoaCurrentControl& getCurrentControl();

    /**
     * @brief Get reference to button control
     * @return MoaButtonControl& Button input producer
     */
    MoaButtonControl& getButtonControl();

    /**
     * @brief Get reference to LED control
     * @return MoaLedControl& LED output controller
     */
    MoaLedControl& getLedControl();

    /**
     * @brief Get reference to state machine manager
     * @return MoaStateMachineManager& Event router and state machine
     */
    MoaStateMachineManager& getStateMachineManager();

    /**
     * @brief Get reference to flash log
     * @return MoaFlashLog& Flash logger
     */
    MoaFlashLog& getFlashLog();

    /**
     * @brief Get the stats queue handle
     * @return QueueHandle_t Stats queue
     */
    QueueHandle_t getStatsQueue();

    /**
     * @brief Get reference to stats aggregator
     * @return MoaStatsAggregator& Stats aggregator
     */
    MoaStatsAggregator& getStatsAggregator();

private:
    // === FreeRTOS resources ===
    QueueHandle_t _eventQueue;
    QueueHandle_t _statsQueue;
    TaskHandle_t _sensorTaskHandle;
    TaskHandle_t _ioTaskHandle;
    TaskHandle_t _controlTaskHandle;
    TaskHandle_t _statsTaskHandle;

    // === Hardware instances ===
    MoaMcpDevice _mcpDevice;
    MoaTempControl _tempControl;
    MoaBattControl _battControl;
    MoaCurrentControl _currentControl;
    MoaButtonControl _buttonControl;
    MoaLedControl _ledControl;
    MoaFlashLog _flashLog;
    ESCController _escController;

    // === Managers ===
    MoaDevicesManager _devicesManager;
    MoaStateMachineManager _stateMachineManager;
    MoaStatsAggregator _statsAggregator;

    /**
     * @brief Initialize I2C bus
     */
    void initI2C();

    /**
     * @brief Initialize all hardware components
     */
    void initHardware();

    /**
     * @brief Apply configuration from Constants.h
     */
    void applyConfiguration();

    /**
     * @brief Create FreeRTOS tasks
     */
    void createTasks();
};
