/**
 * @file MoaTempControl.cpp
 * @brief Implementation of the MoaTempControl class
 * @author Oscar Martinez
 * @date 2025-01-28
 */

#include "MoaTempControl.h"
#include "esp_log.h"

static const char* TAG = "Temp";

MoaTempControl::MoaTempControl(QueueHandle_t eventQueue, uint8_t pin,
                               uint8_t numSamples)
    : _eventQueue(eventQueue)
    , _statsQueue(nullptr)
    , _oneWire(pin)
    , _sensors(&_oneWire)
    , _targetTemp(0.0f)
    , _currentTemp(0.0f)
    , _hysteresis(0.0f)
    , _state(MoaTempState::BELOW_TARGET)
    , _samples(nullptr)
    , _numSamples(0)
    , _sampleIndex(0)
    , _sampleCount(0)
    , _averagedTemp(0.0f)
{
    setNumSamples(numSamples);
}

MoaTempControl::~MoaTempControl() {
    if (_samples != nullptr) {
        delete[] _samples;
        _samples = nullptr;
    }
}

void MoaTempControl::begin() {
    _sensors.begin();
    ESP_LOGD(TAG, "Temperature sensor begin, devices found: %d", _sensors.getDeviceCount());
}

void MoaTempControl::update() {
    _sensors.requestTemperatures();
    _currentTemp = _sensors.getTempCByIndex(0);
    
    // Skip invalid readings (sensor disconnected or error)
    if (_currentTemp == DEVICE_DISCONNECTED_C) {
        ESP_LOGW(TAG, "Temperature sensor disconnected!");
        return;
    }
    
    // Add sample to circular buffer and update average
    addSample(_currentTemp);
    
    // Push stats reading to telemetry queue
    pushStatsReading();
    
    // Only check thresholds if we have enough samples for valid averaging
    if (!isAveragingReady()) {
        return;
    }
    
    // Calculate thresholds
    float upperThreshold = _targetTemp;
    float lowerThreshold = _targetTemp - _hysteresis;
    
    // Check for state transitions and push event
    if (_state == MoaTempState::BELOW_TARGET && _averagedTemp >= upperThreshold) {
        // Crossed UP above target
        _state = MoaTempState::ABOVE_TARGET;
        ESP_LOGW(TAG, "State -> ABOVE_TARGET (avg=%.1fC, target=%.1fC)", _averagedTemp, _targetTemp);
        pushTempEvent(COMMAND_TEMP_CROSSED_ABOVE);
    } else if (_state == MoaTempState::ABOVE_TARGET && _averagedTemp <= lowerThreshold) {
        // Crossed DOWN below (target - hysteresis)
        _state = MoaTempState::BELOW_TARGET;
        ESP_LOGI(TAG, "State -> BELOW_TARGET (avg=%.1fC, lower=%.1fC)", _averagedTemp, lowerThreshold);
        pushTempEvent(COMMAND_TEMP_CROSSED_BELOW);
    }
}

void MoaTempControl::setTargetTemp(float temp) {
    _targetTemp = temp;
}

float MoaTempControl::getTargetTemp() const {
    return _targetTemp;
}

void MoaTempControl::setHysteresis(float hysteresis) {
    _hysteresis = (hysteresis > 0.0f) ? hysteresis : 0.0f;
}

float MoaTempControl::getHysteresis() const {
    return _hysteresis;
}

float MoaTempControl::getCurrentTemp() const {
    return _currentTemp;
}

float MoaTempControl::getAveragedTemp() const {
    return _averagedTemp;
}

MoaTempState MoaTempControl::getState() const {
    return _state;
}

bool MoaTempControl::isAveragingReady() const {
    return _sampleCount >= _numSamples;
}

void MoaTempControl::setNumSamples(uint8_t numSamples) {
    // Clamp to valid range
    if (numSamples < 1) {
        numSamples = 1;
    } else if (numSamples > MOA_TEMP_MAX_SAMPLES) {
        numSamples = MOA_TEMP_MAX_SAMPLES;
    }
    
    // Free existing buffer if any
    if (_samples != nullptr) {
        delete[] _samples;
    }
    
    // Allocate new buffer
    _numSamples = numSamples;
    _samples = new float[_numSamples];
    
    // Reset buffer state
    _sampleIndex = 0;
    _sampleCount = 0;
    _averagedTemp = 0.0f;
    
    // Initialize buffer to zero
    for (uint8_t i = 0; i < _numSamples; i++) {
        _samples[i] = 0.0f;
    }
}

uint8_t MoaTempControl::getNumSamples() const {
    return _numSamples;
}

void MoaTempControl::addSample(float temp) {
    if (_samples == nullptr) {
        return;
    }
    
    // Add sample to circular buffer
    _samples[_sampleIndex] = temp;
    _sampleIndex = (_sampleIndex + 1) % _numSamples;
    
    // Track how many samples we have
    if (_sampleCount < _numSamples) {
        _sampleCount++;
    }
    
    // Update cached average
    _averagedTemp = calculateAverage();
}

float MoaTempControl::calculateAverage() const {
    if (_sampleCount == 0 || _samples == nullptr) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (uint8_t i = 0; i < _sampleCount; i++) {
        sum += _samples[i];
    }
    
    return sum / static_cast<float>(_sampleCount);
}

void MoaTempControl::pushTempEvent(int commandType) {
    if (_eventQueue == nullptr) {
        return;
    }

    ControlCommand cmd;
    cmd.controlType = CONTROL_TYPE_TEMPERATURE;
    cmd.commandType = commandType;
    // Send temperature as int (x10 for one decimal precision, e.g., 25.5°C = 255)
    cmd.value = static_cast<int>(_averagedTemp * 10.0f);

    xQueueSend(_eventQueue, &cmd, 0);  // Don't block if queue is full
}

void MoaTempControl::setStatsQueue(QueueHandle_t statsQueue) {
    _statsQueue = statsQueue;
}

void MoaTempControl::pushStatsReading() {
    if (_statsQueue == nullptr) {
        return;
    }

    StatsReading reading;
    reading.statsType = STATS_TYPE_TEMPERATURE;
    reading.value = static_cast<int32_t>(_averagedTemp * 10.0f);
    reading.timestamp = millis();

    xQueueSend(_statsQueue, &reading, 0);  // Don't block if queue is full
}