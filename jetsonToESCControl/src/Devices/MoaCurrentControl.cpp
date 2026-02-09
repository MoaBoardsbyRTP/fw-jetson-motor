/**
 * @file MoaCurrentControl.cpp
 * @brief Implementation of the MoaCurrentControl class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaCurrentControl.h"
#include "esp_log.h"

static const char* TAG = "Current";

MoaCurrentControl::MoaCurrentControl(QueueHandle_t eventQueue, uint8_t adcPin,
                                     uint8_t numSamples)
    : _eventQueue(eventQueue)
    , _statsQueue(nullptr)
    , _adcPin(adcPin)
    , _adcResolution(12)
    , _sensitivity(0.0066f)           // ACS759-200B: 6.6 mV/A
    , _zeroOffset(1.65f)              // VCC/2 at 3.3V supply
    , _referenceVoltage(3.3f)
    , _overcurrentThreshold(150.0f)   // Default 150A
    , _reverseOvercurrentThreshold(-150.0f)
    , _hysteresis(5.0f)               // Default 5A hysteresis
    , _rawAdc(0)
    , _adcVoltage(0.0f)
    , _currentReading(0.0f)
    , _state(MoaCurrentState::NORMAL)
    , _samples(nullptr)
    , _numSamples(0)
    , _sampleIndex(0)
    , _sampleCount(0)
    , _averagedCurrent(0.0f)
{
    setNumSamples(numSamples);
}

MoaCurrentControl::~MoaCurrentControl() {
    if (_samples != nullptr) {
        delete[] _samples;
        _samples = nullptr;
    }
}

void MoaCurrentControl::begin() {
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_5, GPIO_FLOATING);
    pinMode(_adcPin, INPUT);
    analogReadResolution(_adcResolution);
    ESP_LOGD(TAG, "Current sensor begin (pin=%d, res=%d bits)", _adcPin, _adcResolution);
}

void MoaCurrentControl::update() {
    _rawAdc = analogRead(_adcPin);
    _currentReading = adcToCurrent(_rawAdc);
    
    // Add sample to circular buffer and update average
    addSample(_currentReading);
    
    // Push stats reading to telemetry queue
    pushStatsReading();
    
    // Only check thresholds if we have enough samples for valid averaging
    if (!isAveragingReady()) {
        return;
    }
    
    // Calculate thresholds with hysteresis based on current state
    float overcurrentUp = _overcurrentThreshold;
    float overcurrentDown = _overcurrentThreshold - _hysteresis;
    float reverseUp = _reverseOvercurrentThreshold + _hysteresis;
    float reverseDown = _reverseOvercurrentThreshold;
    
    // Check for state transitions and push events
    MoaCurrentState previousState = _state;
    
    switch (_state) {
        case MoaCurrentState::NORMAL:
            // From NORMAL, can go to OVERCURRENT or REVERSE_OVERCURRENT
            if (_averagedCurrent >= overcurrentUp) {
                _state = MoaCurrentState::OVERCURRENT;
            } else if (_averagedCurrent <= reverseDown) {
                _state = MoaCurrentState::REVERSE_OVERCURRENT;
            }
            break;
            
        case MoaCurrentState::OVERCURRENT:
            // From OVERCURRENT, can only go back to NORMAL
            if (_averagedCurrent <= overcurrentDown) {
                _state = MoaCurrentState::NORMAL;
            }
            break;
            
        case MoaCurrentState::REVERSE_OVERCURRENT:
            // From REVERSE_OVERCURRENT, can only go back to NORMAL
            if (_averagedCurrent >= reverseUp) {
                _state = MoaCurrentState::NORMAL;
            }
            break;
    }
    
    // Push event if state changed
    if (_state != previousState) {
        switch (_state) {
            case MoaCurrentState::NORMAL:
                ESP_LOGI(TAG, "State -> NORMAL (avg=%.1fA)", _averagedCurrent);
                pushCurrentEvent(COMMAND_CURRENT_NORMAL);
                break;
            case MoaCurrentState::OVERCURRENT:
                ESP_LOGW(TAG, "State -> OVERCURRENT (avg=%.1fA, threshold=%.1fA)", _averagedCurrent, _overcurrentThreshold);
                pushCurrentEvent(COMMAND_CURRENT_OVERCURRENT);
                break;
            case MoaCurrentState::REVERSE_OVERCURRENT:
                ESP_LOGW(TAG, "State -> REVERSE_OVERCURRENT (avg=%.1fA, threshold=%.1fA)", _averagedCurrent, _reverseOvercurrentThreshold);
                pushCurrentEvent(COMMAND_CURRENT_REVERSE_OVERCURRENT);
                break;
        }
    }
}

void MoaCurrentControl::setSensitivity(float sensitivity) {
    _sensitivity = (sensitivity > 0.0f) ? sensitivity : 0.0066f;
}

float MoaCurrentControl::getSensitivity() const {
    return _sensitivity;
}

void MoaCurrentControl::setZeroOffset(float offset) {
    _zeroOffset = offset;
}

float MoaCurrentControl::getZeroOffset() const {
    return _zeroOffset;
}

void MoaCurrentControl::setReferenceVoltage(float voltage) {
    _referenceVoltage = (voltage > 0.0f) ? voltage : 3.3f;
}

float MoaCurrentControl::getReferenceVoltage() const {
    return _referenceVoltage;
}

void MoaCurrentControl::setOvercurrentThreshold(float current) {
    _overcurrentThreshold = current;
}

float MoaCurrentControl::getOvercurrentThreshold() const {
    return _overcurrentThreshold;
}

void MoaCurrentControl::setReverseOvercurrentThreshold(float current) {
    _reverseOvercurrentThreshold = current;
}

float MoaCurrentControl::getReverseOvercurrentThreshold() const {
    return _reverseOvercurrentThreshold;
}

void MoaCurrentControl::setHysteresis(float hysteresis) {
    _hysteresis = (hysteresis >= 0.0f) ? hysteresis : 0.0f;
}

float MoaCurrentControl::getHysteresis() const {
    return _hysteresis;
}

uint16_t MoaCurrentControl::getRawAdc() const {
    return _rawAdc;
}

float MoaCurrentControl::getAdcVoltage() const {
    return _adcVoltage;
}

float MoaCurrentControl::getCurrentReading() const {
    return _currentReading;
}

float MoaCurrentControl::getAveragedCurrent() const {
    return _averagedCurrent;
}

MoaCurrentState MoaCurrentControl::getState() const {
    return _state;
}

bool MoaCurrentControl::isAveragingReady() const {
    return _sampleCount >= _numSamples;
}

void MoaCurrentControl::setNumSamples(uint8_t numSamples) {
    // Clamp to valid range
    if (numSamples < 1) {
        numSamples = 1;
    } else if (numSamples > MOA_CURRENT_MAX_SAMPLES) {
        numSamples = MOA_CURRENT_MAX_SAMPLES;
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
    _averagedCurrent = 0.0f;
    
    // Initialize buffer to zero
    for (uint8_t i = 0; i < _numSamples; i++) {
        _samples[i] = 0.0f;
    }
}

uint8_t MoaCurrentControl::getNumSamples() const {
    return _numSamples;
}

void MoaCurrentControl::setAdcResolution(uint8_t bits) {
    _adcResolution = bits;
    analogReadResolution(_adcResolution);
}

uint8_t MoaCurrentControl::getAdcResolution() const {
    return _adcResolution;
}

void MoaCurrentControl::addSample(float current) {
    if (_samples == nullptr) {
        return;
    }
    
    // Add sample to circular buffer
    _samples[_sampleIndex] = current;
    _sampleIndex = (_sampleIndex + 1) % _numSamples;
    
    // Track how many samples we have
    if (_sampleCount < _numSamples) {
        _sampleCount++;
    }
    
    // Update cached average
    _averagedCurrent = calculateAverage();
}

float MoaCurrentControl::calculateAverage() const {
    if (_sampleCount == 0 || _samples == nullptr) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (uint8_t i = 0; i < _sampleCount; i++) {
        sum += _samples[i];
    }
    
    return sum / static_cast<float>(_sampleCount);
}

float MoaCurrentControl::adcToCurrent(uint16_t rawAdc) {
    // Convert ADC reading to voltage at ADC pin
    float maxAdcValue = static_cast<float>((1 << _adcResolution) - 1);
    _adcVoltage = (static_cast<float>(rawAdc) / maxAdcValue) * _referenceVoltage;
    
    // Convert voltage to current using sensitivity and offset
    // Current = (Vadc - Voffset) / Sensitivity
    // Positive current when Vadc > Voffset, negative when Vadc < Voffset
    return (_adcVoltage - _zeroOffset) / _sensitivity;
}

void MoaCurrentControl::pushCurrentEvent(int commandType) {
    if (_eventQueue == nullptr) {
        return;
    }

    ControlCommand cmd;
    cmd.controlType = CONTROL_TYPE_CURRENT;
    cmd.commandType = commandType;
    // Send current as int (x10 for one decimal precision, e.g., 125.5A = 1255)
    cmd.value = static_cast<int>(_averagedCurrent * 10.0f);

    xQueueSend(_eventQueue, &cmd, 0);  // Don't block if queue is full
}

void MoaCurrentControl::setStatsQueue(QueueHandle_t statsQueue) {
    _statsQueue = statsQueue;
}

void MoaCurrentControl::pushStatsReading() {
    if (_statsQueue == nullptr) {
        return;
    }

    StatsReading reading;
    reading.statsType = STATS_TYPE_CURRENT;
    reading.value = static_cast<int32_t>(_averagedCurrent * 10.0f);  // x10 for precision
    reading.timestamp = millis();

    xQueueSend(_statsQueue, &reading, 0);  // Don't block if queue is full
}
