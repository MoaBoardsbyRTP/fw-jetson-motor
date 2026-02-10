/**
 * @file MoaMainUnit.cpp
 * @brief Implementation of the MoaMainUnit class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaMainUnit.h"
#include "Tasks.h"
#include "esp_log.h"

static const char* TAG = "MainUnit";

MoaMainUnit::MoaMainUnit()
    : _eventQueue(nullptr)
    , _statsQueue(nullptr)
    , _sensorTaskHandle(nullptr)
    , _ioTaskHandle(nullptr)
    , _controlTaskHandle(nullptr)
    , _statsTaskHandle(nullptr)
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
    ESP_LOGI(TAG, "Moa ESC Controller starting...");

    // Create event queue FIRST (producers need it)
    _eventQueue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(ControlCommand));
    if (_eventQueue == nullptr) {
        ESP_LOGE(TAG, "Failed to create event queue!");
        return;
    }
    ESP_LOGD(TAG, "Event queue created (size=%d)", EVENT_QUEUE_SIZE);

    // Create stats queue for telemetry
    _statsQueue = xQueueCreate(STATS_QUEUE_SIZE, sizeof(StatsReading));
    if (_statsQueue == nullptr) {
        ESP_LOGE(TAG, "Failed to create stats queue!");
        return;
    }
    ESP_LOGD(TAG, "Stats queue created (size=%d)", STATS_QUEUE_SIZE);

    // Initialize stats aggregator
    _statsAggregator.begin();

    // Set event queue on all producers (queue was nullptr at construction time)
    _tempControl.setEventQueue(_eventQueue);
    _battControl.setEventQueue(_eventQueue);
    _currentControl.setEventQueue(_eventQueue);
    _buttonControl.setEventQueue(_eventQueue);

    // Set stats queue on sensor producers
    _tempControl.setStatsQueue(_statsQueue);
    _battControl.setStatsQueue(_statsQueue);
    _currentControl.setStatsQueue(_statsQueue);

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

    ESP_LOGI(TAG, "Moa ESC Controller ready.");
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

QueueHandle_t MoaMainUnit::getStatsQueue() {
    return _statsQueue;
}

MoaStatsAggregator& MoaMainUnit::getStatsAggregator() {
    return _statsAggregator;
}

void MoaMainUnit::initI2C() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    ESP_LOGI(TAG, "I2C initialized (SDA=%d, SCL=%d)", PIN_I2C_SDA, PIN_I2C_SCL);
}

void MoaMainUnit::initHardware() {
    // Initialize MCP23018
    if (_mcpDevice.begin(&Wire)) {
        ESP_LOGI(TAG, "MCP23018 initialized");
    } else {
        ESP_LOGW(TAG, "MCP23018 initialization failed!");
    }

    // Initialize temperature sensor
    _tempControl.begin();
    ESP_LOGI(TAG, "Temperature sensor initialized");

    // Initialize battery monitor
    _battControl.begin();
    ESP_LOGI(TAG, "Battery monitor initialized");

    // Initialize current sensor
    _currentControl.begin();
    ESP_LOGI(TAG, "Current sensor initialized");

    // Initialize button input with interrupt mode enabled
    _buttonControl.begin(true);  // Interrupt-driven mode
    ESP_LOGI(TAG, "Button input initialized (interrupt mode)");

    // Initialize LED output
    _ledControl.begin();
    ESP_LOGI(TAG, "LED output initialized");

    // Initialize flash log
    if (_flashLog.begin()) {
        ESP_LOGI(TAG, "Flash log initialized");
    } else {
        ESP_LOGW(TAG, "Flash log initialization failed!");
    }

    // Initialize ESC
    _escController.begin();
    ESP_LOGI(TAG, "ESC controller initialized (pin=%d, freq=%d)", PIN_ESC_PWM, ESC_PWM_FREQUENCY);
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

    ESP_LOGI(TAG, "Configuration applied");
    ESP_LOGD(TAG, "  Batt: divider=%.2f, low=%.2fV, high=%.2fV, hyst=%.2fV", BATT_DIVIDER_RATIO, BATT_THRESHOLD_LOW, BATT_THRESHOLD_HIGH, BATT_HYSTERESIS);
    ESP_LOGD(TAG, "  Current: sens=%.4f, offset=%.2f, OC=%.1fA, rev=%.1fA, hyst=%.1fA", CURRENT_SENSOR_SENSITIVITY, CURRENT_SENSOR_OFFSET, CURRENT_THRESHOLD_OVERCURRENT, CURRENT_THRESHOLD_REVERSE, CURRENT_HYSTERESIS);
    ESP_LOGD(TAG, "  Temp: target=%.1fC, hyst=%.1fC", TEMP_THRESHOLD_TARGET, TEMP_HYSTERESIS);
    ESP_LOGD(TAG, "  Button: debounce=%dms, longPress=%dms", BUTTON_DEBOUNCE_MS, BUTTON_LONG_PRESS_MS);
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
    ESP_LOGI(TAG, "SensorTask created (stack=%d, prio=%d)", TASK_STACK_SENSOR, TASK_PRIORITY_SENSOR);

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
    ESP_LOGI(TAG, "IOTask created (stack=%d, prio=%d)", TASK_STACK_IO, TASK_PRIORITY_IO);

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
    ESP_LOGI(TAG, "ControlTask created (stack=%d, prio=%d)", TASK_STACK_CONTROL, TASK_PRIORITY_CONTROL);

    // Create StatsTask
    xTaskCreatePinnedToCore(
        StatsTask,
        "StatsTask",
        TASK_STACK_STATS,
        this,
        TASK_PRIORITY_STATS,
        &_statsTaskHandle,
        0
    );
    ESP_LOGI(TAG, "StatsTask created (stack=%d, prio=%d)", TASK_STACK_STATS, TASK_PRIORITY_STATS);
}
