/**
 * @file TempControl.cpp
 * @brief Implementation of the TempControl class
 * @author Oscar Martinez
 * @date 2025-01-28
 */

#include "TempControl.h"

TempControl::TempControl(uint8_t pin, uint8_t numSamples)
    : _oneWire(pin)
    , _sensors(&_oneWire)
    , _targetTemp(0.0f)
    , _currentTemp(0.0f)
    , _hysteresis(0.0f)
    , _callback(nullptr)
    , _state(TempState::BELOW_TARGET)
    , _samples(nullptr)
    , _numSamples(0)
    , _sampleIndex(0)
    , _sampleCount(0)
    , _averagedTemp(0.0f)
{
    setNumSamples(numSamples);
}

TempControl::~TempControl() {
    if (_samples != nullptr) {
        delete[] _samples;
        _samples = nullptr;
    }
}

void TempControl::begin() {
    _sensors.begin();
}

void TempControl::update() {
    _sensors.requestTemperatures();
    _currentTemp = _sensors.getTempCByIndex(0);
    
    // Skip invalid readings (sensor disconnected or error)
    if (_currentTemp == DEVICE_DISCONNECTED_C) {
        return;
    }
    
    // Add sample to circular buffer and update average
    addSample(_currentTemp);
    
    // Only check thresholds if we have enough samples for valid averaging
    if (!isAveragingReady()) {
        return;
    }
    
    // Calculate thresholds
    float upperThreshold = _targetTemp;
    float lowerThreshold = _targetTemp - _hysteresis;
    
    // Check for state transitions and fire callback
    if (_state == TempState::BELOW_TARGET && _averagedTemp >= upperThreshold) {
        // Crossed UP above target
        _state = TempState::ABOVE_TARGET;
        if (_callback != nullptr) {
            _callback(_averagedTemp, true);
        }
    } else if (_state == TempState::ABOVE_TARGET && _averagedTemp <= lowerThreshold) {
        // Crossed DOWN below (target - hysteresis)
        _state = TempState::BELOW_TARGET;
        if (_callback != nullptr) {
            _callback(_averagedTemp, false);
        }
    }
}

void TempControl::setTargetTemp(float temp) {
    _targetTemp = temp;
}

float TempControl::getTargetTemp() const {
    return _targetTemp;
}

void TempControl::setHysteresis(float hysteresis) {
    _hysteresis = (hysteresis > 0.0f) ? hysteresis : 0.0f;
}

float TempControl::getHysteresis() const {
    return _hysteresis;
}

void TempControl::setCallback(void (*callback)(float, bool)) {
    _callback = callback;
}

float TempControl::getCurrentTemp() const {
    return _currentTemp;
}

float TempControl::getAveragedTemp() const {
    return _averagedTemp;
}

TempState TempControl::getState() const {
    return _state;
}

bool TempControl::isAveragingReady() const {
    return _sampleCount >= _numSamples;
}

void TempControl::setNumSamples(uint8_t numSamples) {
    // Clamp to valid range
    if (numSamples < 1) {
        numSamples = 1;
    } else if (numSamples > TEMP_CONTROL_MAX_SAMPLES) {
        numSamples = TEMP_CONTROL_MAX_SAMPLES;
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

uint8_t TempControl::getNumSamples() const {
    return _numSamples;
}

void TempControl::addSample(float temp) {
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

float TempControl::calculateAverage() const {
    if (_sampleCount == 0 || _samples == nullptr) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (uint8_t i = 0; i < _sampleCount; i++) {
        sum += _samples[i];
    }
    
    return sum / static_cast<float>(_sampleCount);
}