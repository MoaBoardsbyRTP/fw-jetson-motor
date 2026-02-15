/*!
 * @file ESCController.cpp
 * @brief ESCController class implementation for controlling ESCs via PWM
 * @author Oscar Martinez
 * @date 2025-01-28
 */

#include "ESCController.h"
#include "esp_log.h"

static const char* TAG = "ESC";

ESCController::ESCController(uint8_t pin, uint8_t channel, uint16_t frequency){
    _pin = pin;
    _channel = channel;
    _frequency = frequency;
    _resolution = 10;
    uint32_t periodUs = 1000000UL / _frequency;  // 20000µs at 50Hz
    uint16_t maxDuty = (1 << _resolution) - 1;    // 1023 for 10-bit
    _minThrottle = (uint16_t)((uint32_t)ESC_PULSE_MIN_US * maxDuty / periodUs);  // ~51 for 1ms
    _maxThrottle = (uint16_t)((uint32_t)ESC_PULSE_MAX_US * maxDuty / periodUs);  // ~102 for 2ms
    _currentThrottle = _minThrottle;
    _targetThrottle = _minThrottle;
    _throttle = _minThrottle;
    _rampTime = 10;
    _rampStep = 1;
    _ramping = false;
    _rampRate = ESC_RAMP_RATE;
    _tickPeriodMs = TASK_IO_PERIOD_MS;
}

void ESCController::begin(){
    ledcSetup(_channel, _frequency, _resolution);
    ledcAttachPin(_pin, _channel);
    ESP_LOGI(TAG, "ESC begin (pin=%d, ch=%d, freq=%d, res=%d)", _pin, _channel, _frequency, _resolution);
    stop();
}

void ESCController::writeThrottle(){
    ESP_LOGD(TAG, "ESC writeThrottle (duty=%d, min=%d, max=%d)", _throttle, _minThrottle, _maxThrottle);
    ledcWrite(_channel, _throttle);
}

void ESCController::stop(){
    ESP_LOGI(TAG, "ESC stop");
    _ramping = false;
    setThrottle(_minThrottle);
}

void ESCController::updateThrottle(){
    if(!_ramping){
        return;
    }

    if(_rampStep > 0){
        _currentThrottle += _rampStep;
        if(_currentThrottle >= _targetThrottle){
            _currentThrottle = _targetThrottle;
            _ramping = false;
            ESP_LOGI(TAG, "Ramp complete (throttle=%d)", _currentThrottle);
        }
    } else if(_rampStep < 0){
        _currentThrottle += _rampStep;
        if(_currentThrottle <= _targetThrottle){
            _currentThrottle = _targetThrottle;
            _ramping = false;
            ESP_LOGI(TAG, "Ramp complete (throttle=%d)", _currentThrottle);
        }
    } else {
        _currentThrottle = _targetThrottle;
        _ramping = false;
    }

    _throttle = _currentThrottle;
    writeThrottle();
}

bool ESCController::isRamping() const{
    return _ramping;
}

void ESCController::setThrottle(uint16_t throttle){
    if(throttle < _minThrottle){
        _throttle = _minThrottle;
    } else if(throttle > _maxThrottle){
        _throttle = _maxThrottle;
    } else {
        _throttle = throttle;
    }
    _currentThrottle = _throttle;
    writeThrottle();
}

uint16_t ESCController::getCurrentThrottle() const{
    return _currentThrottle;
}

void ESCController::setThrottlePercent(uint8_t percent){
    if (percent > 100) {
        percent = 100;
    }
    uint16_t range = _maxThrottle - _minThrottle;
    uint16_t targetDuty = _minThrottle + (uint16_t)((uint32_t)percent * range / 100);
    float currentPercent = (float)(_currentThrottle - _minThrottle) * 100.0f / range;
    float deltaPercent = abs((float)percent - currentPercent);
    uint16_t rampSteps = (uint16_t)(deltaPercent * 1000.0f / (_rampRate * _tickPeriodMs));
    if (rampSteps < 1) rampSteps = 1;

    ESP_LOGI(TAG, "Throttle ramp to %d%% (duty=%d, range=%d-%d, steps=%d)", percent, targetDuty, _minThrottle, _maxThrottle, rampSteps);
    setRampThrottle(rampSteps, targetDuty);
}

void ESCController::setThrottleDuty(uint16_t duty){
    if (duty < _minThrottle) duty = _minThrottle;
    if (duty > _maxThrottle) duty = _maxThrottle;

    uint16_t range = _maxThrottle - _minThrottle;
    float currentPercent = (float)(_currentThrottle - _minThrottle) * 100.0f / range;
    float targetPercent  = (float)(duty - _minThrottle) * 100.0f / range;
    float deltaPercent   = abs(targetPercent - currentPercent);
    uint16_t rampSteps   = (uint16_t)(deltaPercent * 1000.0f / (_rampRate * _tickPeriodMs));
    if (rampSteps < 1) rampSteps = 1;

    ESP_LOGI(TAG, "Throttle ramp to duty=%d (range=%d-%d, steps=%d)", duty, _minThrottle, _maxThrottle, rampSteps);
    setRampThrottle(rampSteps, duty);
}

void ESCController::setRampRate(float ratePercentPerSec){
    _rampRate = (ratePercentPerSec > 0) ? ratePercentPerSec : 1.0f;
}

void ESCController::setTickPeriod(uint16_t periodMs){
    _tickPeriodMs = (periodMs > 0) ? periodMs : 1;
}

void ESCController::setRampThrottle(uint16_t rampTime, uint16_t targetThrottle){
    ESP_LOGI(TAG, "Ramp set: target=%d, time=%d, current=%d", targetThrottle, rampTime, _currentThrottle);
    _targetThrottle = targetThrottle;
    
    if(_targetThrottle > _maxThrottle){
        _targetThrottle = _maxThrottle;
    }
    
    if(_targetThrottle == _currentThrottle){
        _ramping = false;
        return;
    }
    
    _rampTime = (rampTime > 0) ? rampTime : 1;
    _rampStep = (int16_t)(_targetThrottle - _currentThrottle) / (int16_t)_rampTime;
    
    if(_rampStep == 0){
        _rampStep = (_targetThrottle > _currentThrottle) ? 1 : -1;
    }
    
    _ramping = true;
}