/**
 * @file MoaBattControl.cpp
 * @brief Implementation of the MoaBattControl class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaBattControl.h"

MoaBattControl::MoaBattControl(QueueHandle_t eventQueue, uint8_t adcPin,
                               uint8_t numSamples)
    : _eventQueue(eventQueue)
    , _statsQueue(nullptr)
    , _adcPin(adcPin)
    , _adcResolution(12)
    , _dividerRatio(1.0f)
    , _referenceVoltage(3.3f)
    , _lowThreshold(3.3f)
    , _highThreshold(4.0f)
    , _hysteresis(0.1f)
    , _rawAdc(0)
    , _currentVoltage(0.0f)
    , _level(MoaBattLevel::BATT_MEDIUM)
    , _samples(nullptr)
    , _numSamples(0)
    , _sampleIndex(0)
    , _sampleCount(0)
    , _averagedVoltage(0.0f)
{
    setNumSamples(numSamples);
}

MoaBattControl::~MoaBattControl() {
    if (_samples != nullptr) {
        delete[] _samples;
        _samples = nullptr;
    }
}

void MoaBattControl::begin() {
    pinMode(_adcPin, INPUT);
    analogReadResolution(_adcResolution);
}

void MoaBattControl::update() {
    _rawAdc = analogRead(_adcPin);
    _currentVoltage = adcToVoltage(_rawAdc);
    
    // Add sample to circular buffer and update average
    addSample(_currentVoltage);
    
    // Push stats reading to telemetry queue
    pushStatsReading();
    
    // Only check thresholds if we have enough samples for valid averaging
    if (!isAveragingReady()) {
        return;
    }
    
    // Calculate thresholds with hysteresis based on current state
    float lowThreshUp = _lowThreshold + _hysteresis;
    float lowThreshDown = _lowThreshold;
    float highThreshUp = _highThreshold;
    float highThreshDown = _highThreshold - _hysteresis;
    
    // Check for state transitions and push events
    MoaBattLevel previousLevel = _level;
    
    switch (_level) {
        case MoaBattLevel::BATT_LOW:
            // From LOW, can only go to MEDIUM (crossing up above low threshold)
            if (_averagedVoltage >= lowThreshUp) {
                _level = MoaBattLevel::BATT_MEDIUM;
            }
            break;
            
        case MoaBattLevel::BATT_MEDIUM:
            // From MEDIUM, can go to LOW or HIGH
            if (_averagedVoltage <= lowThreshDown) {
                _level = MoaBattLevel::BATT_LOW;
            } else if (_averagedVoltage >= highThreshUp) {
                _level = MoaBattLevel::BATT_HIGH;
            }
            break;
            
        case MoaBattLevel::BATT_HIGH:
            // From HIGH, can only go to MEDIUM (crossing down below high threshold)
            if (_averagedVoltage <= highThreshDown) {
                _level = MoaBattLevel::BATT_MEDIUM;
            }
            break;
    }
    
    // Push event if level changed
    if (_level != previousLevel) {
        switch (_level) {
            case MoaBattLevel::BATT_LOW:
                pushBattEvent(COMMAND_BATT_LEVEL_LOW);
                break;
            case MoaBattLevel::BATT_MEDIUM:
                pushBattEvent(COMMAND_BATT_LEVEL_MEDIUM);
                break;
            case MoaBattLevel::BATT_HIGH:
                pushBattEvent(COMMAND_BATT_LEVEL_HIGH);
                break;
        }
    }
}

void MoaBattControl::setDividerRatio(float ratio) {
    _dividerRatio = (ratio >= 1.0f) ? ratio : 1.0f;
}

float MoaBattControl::getDividerRatio() const {
    return _dividerRatio;
}

void MoaBattControl::setReferenceVoltage(float voltage) {
    _referenceVoltage = (voltage > 0.0f) ? voltage : 3.3f;
}

float MoaBattControl::getReferenceVoltage() const {
    return _referenceVoltage;
}

void MoaBattControl::setLowThreshold(float voltage) {
    _lowThreshold = voltage;
}

float MoaBattControl::getLowThreshold() const {
    return _lowThreshold;
}

void MoaBattControl::setHighThreshold(float voltage) {
    _highThreshold = voltage;
}

float MoaBattControl::getHighThreshold() const {
    return _highThreshold;
}

void MoaBattControl::setHysteresis(float hysteresis) {
    _hysteresis = (hysteresis >= 0.0f) ? hysteresis : 0.0f;
}

float MoaBattControl::getHysteresis() const {
    return _hysteresis;
}

uint16_t MoaBattControl::getRawAdc() const {
    return _rawAdc;
}

float MoaBattControl::getCurrentVoltage() const {
    return _currentVoltage;
}

float MoaBattControl::getAveragedVoltage() const {
    return _averagedVoltage;
}

MoaBattLevel MoaBattControl::getLevel() const {
    return _level;
}

bool MoaBattControl::isAveragingReady() const {
    return _sampleCount >= _numSamples;
}

void MoaBattControl::setNumSamples(uint8_t numSamples) {
    // Clamp to valid range
    if (numSamples < 1) {
        numSamples = 1;
    } else if (numSamples > MOA_BATT_MAX_SAMPLES) {
        numSamples = MOA_BATT_MAX_SAMPLES;
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
    _averagedVoltage = 0.0f;
    
    // Initialize buffer to zero
    for (uint8_t i = 0; i < _numSamples; i++) {
        _samples[i] = 0.0f;
    }
}

uint8_t MoaBattControl::getNumSamples() const {
    return _numSamples;
}

void MoaBattControl::setAdcResolution(uint8_t bits) {
    _adcResolution = bits;
    analogReadResolution(_adcResolution);
}

uint8_t MoaBattControl::getAdcResolution() const {
    return _adcResolution;
}

void MoaBattControl::addSample(float voltage) {
    if (_samples == nullptr) {
        return;
    }
    
    // Add sample to circular buffer
    _samples[_sampleIndex] = voltage;
    _sampleIndex = (_sampleIndex + 1) % _numSamples;
    
    // Track how many samples we have
    if (_sampleCount < _numSamples) {
        _sampleCount++;
    }
    
    // Update cached average
    _averagedVoltage = calculateAverage();
}

float MoaBattControl::calculateAverage() const {
    if (_sampleCount == 0 || _samples == nullptr) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (uint8_t i = 0; i < _sampleCount; i++) {
        sum += _samples[i];
    }
    
    return sum / static_cast<float>(_sampleCount);
}

float MoaBattControl::adcToVoltage(uint16_t rawAdc) const {
    // Convert ADC reading to voltage at ADC pin
    float maxAdcValue = static_cast<float>((1 << _adcResolution) - 1);
    float adcVoltage = (static_cast<float>(rawAdc) / maxAdcValue) * _referenceVoltage;
    
    // Apply voltage divider ratio to get actual battery voltage
    return adcVoltage * _dividerRatio;
}

void MoaBattControl::pushBattEvent(int commandType) {
    if (_eventQueue == nullptr) {
        return;
    }

    ControlCommand cmd;
    cmd.controlType = CONTROL_TYPE_BATTERY;
    cmd.commandType = commandType;
    // Send voltage as int in millivolts (e.g., 3.85V = 3850)
    cmd.value = static_cast<int>(_averagedVoltage * 1000.0f);

    xQueueSend(_eventQueue, &cmd, 0);  // Don't block if queue is full
}

void MoaBattControl::setStatsQueue(QueueHandle_t statsQueue) {
    _statsQueue = statsQueue;
}

void MoaBattControl::pushStatsReading() {
    if (_statsQueue == nullptr) {
        return;
    }

    StatsReading reading;
    reading.statsType = STATS_TYPE_BATTERY;
    reading.value = static_cast<int32_t>(_averagedVoltage * 1000.0f);  // millivolts
    reading.timestamp = millis();

    xQueueSend(_statsQueue, &reading, 0);  // Don't block if queue is full
}
