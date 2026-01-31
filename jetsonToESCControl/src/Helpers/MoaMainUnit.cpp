/**
 * @file MoaMainUnit.cpp
 * @brief Implementation of the MoaMainUnit class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaMainUnit.h"
#include "Tasks.h"

MoaMainUnit::MoaMainUnit()
    : _eventQueue(nullptr)
    , _sensorTaskHandle(nullptr)
    , _ioTaskHandle(nullptr)
    , _controlTaskHandle(nullptr)
    , _mcpDevice(MCP23018_I2C_ADDR)
    , _tempControl(_eventQueue, PIN_TEMP_SENSE)
    , _battControl(_eventQueue, PIN_BATT_LEVEL_SENSE)
    , _currentControl(_eventQueue, PIN_CURRENT_SENSE)
    , _buttonControl(_eventQueue, _mcpDevice, PIN_I2C_INT_A)
    , _ledControl(_mcpDevice)
    , _flashLog()
    , _escController(PIN_ESC_PWM, 0, ESC_PWM_FREQUENCY)
    , _devicesManager(_ledControl, _escController, _flashLog)
    , _stateMachineManager(_devicesManager)
{
}

MoaMainUnit::~MoaMainUnit() {
    // Tasks and queue are managed by FreeRTOS
}

void MoaMainUnit::begin() {
    // Initialize Serial
    Serial.begin(115200);
    Serial.println("Moa ESC Controller starting...");

    // Create event queue FIRST (producers need it)
    _eventQueue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(ControlCommand));
    if (_eventQueue == nullptr) {
        Serial.println("ERROR: Failed to create event queue!");
        return;
    }

    // Update queue handle in producers (they were constructed with nullptr)
    // Note: This requires adding setEventQueue() methods to producers
    // For now, we'll reinitialize them after queue creation

    // Initialize I2C
    initI2C();

    // Initialize hardware
    initHardware();

    // Apply configuration
    applyConfiguration();

    // Set initial state
    _stateMachineManager.setInitialState();

    // Create FreeRTOS tasks
    createTasks();

    // Log boot event
    _flashLog.logSystem(LOG_SYS_BOOT);

    Serial.println("Moa ESC Controller ready.");
}

QueueHandle_t MoaMainUnit::getEventQueue() {
    return _eventQueue;
}

MoaTempControl& MoaMainUnit::getTempControl() {
    return _tempControl;
}

MoaBattControl& MoaMainUnit::getBattControl() {
    return _battControl;
}

MoaCurrentControl& MoaMainUnit::getCurrentControl() {
    return _currentControl;
}

MoaButtonControl& MoaMainUnit::getButtonControl() {
    return _buttonControl;
}

MoaLedControl& MoaMainUnit::getLedControl() {
    return _ledControl;
}

MoaStateMachineManager& MoaMainUnit::getStateMachineManager() {
    return _stateMachineManager;
}

MoaFlashLog& MoaMainUnit::getFlashLog() {
    return _flashLog;
}

void MoaMainUnit::initI2C() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Serial.println("I2C initialized.");
}

void MoaMainUnit::initHardware() {
    // Initialize MCP23018
    if (_mcpDevice.begin(&Wire)) {
        Serial.println("MCP23018 initialized.");
    } else {
        Serial.println("WARNING: MCP23018 initialization failed!");
    }

    // Initialize temperature sensor
    _tempControl.begin();
    Serial.println("Temperature sensor initialized.");

    // Initialize battery monitor
    _battControl.begin();
    Serial.println("Battery monitor initialized.");

    // Initialize current sensor
    _currentControl.begin();
    Serial.println("Current sensor initialized.");

    // Initialize button input
    _buttonControl.begin(false);  // Polling mode (no interrupt)
    Serial.println("Button input initialized.");

    // Initialize LED output
    _ledControl.begin();
    Serial.println("LED output initialized.");

    // Initialize flash log
    if (_flashLog.begin()) {
        Serial.println("Flash log initialized.");
    } else {
        Serial.println("WARNING: Flash log initialization failed!");
    }

    // Initialize ESC
    _escController.begin();
    Serial.println("ESC controller initialized.");
}

void MoaMainUnit::applyConfiguration() {
    // Battery configuration
    _battControl.setDividerRatio(BATT_DIVIDER_RATIO);
    _battControl.setHighThreshold(BATT_THRESHOLD_HIGH);
    _battControl.setLowThreshold(BATT_THRESHOLD_LOW);
    _battControl.setHysteresis(BATT_HYSTERESIS);

    // Current sensor configuration
    _currentControl.setSensitivity(CURRENT_SENSOR_SENSITIVITY);
    _currentControl.setZeroOffset(CURRENT_SENSOR_OFFSET);
    _currentControl.setOvercurrentThreshold(CURRENT_THRESHOLD_OVERCURRENT);
    _currentControl.setReverseOvercurrentThreshold(CURRENT_THRESHOLD_REVERSE);
    _currentControl.setHysteresis(CURRENT_HYSTERESIS);

    // Temperature configuration
    _tempControl.setTargetTemp(TEMP_THRESHOLD_TARGET);
    _tempControl.setHysteresis(TEMP_HYSTERESIS);

    // Button configuration
    _buttonControl.setDebounceTime(BUTTON_DEBOUNCE_MS);
    _buttonControl.setLongPressTime(BUTTON_LONG_PRESS_MS);
    _buttonControl.enableLongPress(true);

    // Flash log configuration
    _flashLog.setFlushInterval(LOG_FLUSH_INTERVAL_MS);

    Serial.println("Configuration applied.");
}

void MoaMainUnit::createTasks() {
    // Create SensorTask
    xTaskCreatePinnedToCore(
        SensorTask,
        "SensorTask",
        TASK_STACK_SENSOR,
        this,  // Pass MoaMainUnit pointer
        TASK_PRIORITY_SENSOR,
        &_sensorTaskHandle,
        0  // Core 0 (ESP32-C3 is single-core)
    );
    Serial.println("SensorTask created.");

    // Create IOTask
    xTaskCreatePinnedToCore(
        IOTask,
        "IOTask",
        TASK_STACK_IO,
        this,
        TASK_PRIORITY_IO,
        &_ioTaskHandle,
        0
    );
    Serial.println("IOTask created.");

    // Create ControlTask
    xTaskCreatePinnedToCore(
        ControlTask,
        "ControlTask",
        TASK_STACK_CONTROL,
        this,
        TASK_PRIORITY_CONTROL,
        &_controlTaskHandle,
        0
    );
    Serial.println("ControlTask created.");
}
