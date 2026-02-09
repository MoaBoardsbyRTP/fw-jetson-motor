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
    _minThrottle = 0;
    _maxThrottle = 1023;
    _currentThrottle = 0;
    _targetThrottle = 0;
    _throttle = 0;
    _rampTime = 10;
    _rampStep = 1;
    _ramping = false;
}

void ESCController::begin(){
    ledcSetup(_channel, _frequency, _resolution);
    ledcAttachPin(_pin, _channel);
    ESP_LOGI(TAG, "ESC begin (pin=%d, ch=%d, freq=%d, res=%d)", _pin, _channel, _frequency, _resolution);
}

void ESCController::writeThrottle(){
    ledcWrite(_channel, _throttle);
}

void ESCController::stop(){
    ESP_LOGI(TAG, "ESC stop");
    setThrottle(_minThrottle);
    writeThrottle();
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
            ESP_LOGD(TAG, "Ramp up complete (throttle=%d)", _currentThrottle);
        }
    } else if(_rampStep < 0){
        _currentThrottle += _rampStep;
        if(_currentThrottle <= _targetThrottle){
            _currentThrottle = _targetThrottle;
            _ramping = false;
            ESP_LOGD(TAG, "Ramp down complete (throttle=%d)", _currentThrottle);
        }
    } else {
        _currentThrottle = _targetThrottle;
        _ramping = false;
    }

    setThrottle(_currentThrottle);
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