/**
 * @file Ds18b20TemperatureSensor.cpp
 * @brief Implementation of Ds18b20TemperatureSensor
 * @author Oscar Martinez
 * @date 2026-07-03
 */

#include "Ds18b20TemperatureSensor.h"
#include "esp_log.h"

static const char* TAG = "Ds18b20Sensor";

Ds18b20TemperatureSensor::Ds18b20TemperatureSensor(uint8_t pin)
    : _oneWire(pin)
    , _sensors(&_oneWire)
    , _convState(ConvState::IDLE)
    , _convRequestTime(0)
    , _convDelayMs(750)
{
}

void Ds18b20TemperatureSensor::begin() {
    _sensors.begin();
    _sensors.setWaitForConversion(false);
    _convDelayMs = _sensors.millisToWaitForConversion();
    ESP_LOGI(TAG, "DS18B20 begin, devices=%d, convMs=%d",
             _sensors.getDeviceCount(), _convDelayMs);
}

bool Ds18b20TemperatureSensor::readCelsius(float& outCelsius) {
    switch (_convState) {
        case ConvState::IDLE:
            // Start a non-blocking conversion request
            _sensors.requestTemperatures();
            _convRequestTime = millis();
            _convState = ConvState::WAITING;
            return false;  // Not ready yet, come back next call

        case ConvState::WAITING:
            // Check if conversion time has elapsed
            if ((millis() - _convRequestTime) < _convDelayMs) {
                return false;  // Not ready yet
            }
            // Conversion done — read result and fall through
            _convState = ConvState::IDLE;
            break;
    }

    outCelsius = _sensors.getTempCByIndex(0);
    return true;
}
