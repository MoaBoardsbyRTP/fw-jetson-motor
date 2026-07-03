/**
 * @file NtcTemperatureSensor.cpp
 * @brief Implementation of NtcTemperatureSensor
 * @author Oscar Martinez
 * @date 2026-07-03
 */

#include "NtcTemperatureSensor.h"
#include "esp_log.h"

static const char* TAG = "NtcSensor";

NtcTemperatureSensor::NtcTemperatureSensor(uint8_t pin,
                                           float referenceResistance,
                                           float nominalResistance,
                                           float nominalTempC,
                                           float betaCoefficient,
                                           uint16_t adcVrefMv)
    : _pin(pin)
    , _referenceResistance(referenceResistance)
    , _nominalResistance(nominalResistance)
    , _nominalTempC(nominalTempC)
    , _betaCoefficient(betaCoefficient)
    , _adcVrefMv(adcVrefMv)
    , _thermistor(nullptr)
{
}

NtcTemperatureSensor::~NtcTemperatureSensor() {
    if (_thermistor != nullptr) {
        delete _thermistor;
        _thermistor = nullptr;
    }
}

void NtcTemperatureSensor::begin() {
    if (_thermistor != nullptr) {
        delete _thermistor;
    }
    _thermistor = new NTC_Thermistor_ESP32(
        _pin,
        _referenceResistance,
        _nominalResistance,
        _nominalTempC,
        _betaCoefficient,
        _adcVrefMv
    );
    ESP_LOGI(TAG, "NTC sensor begin on pin %d (Rref=%.0f, Rnom=%.0f, Tnom=%.0f, Beta=%.0f, Vref=%umV)",
             _pin, _referenceResistance, _nominalResistance, _nominalTempC, _betaCoefficient, _adcVrefMv);
}

bool NtcTemperatureSensor::readCelsius(float& outCelsius) {
    if (_thermistor == nullptr) {
        return false;
    }
    // Single blocking ADC read + math — always ready, no state machine needed.
    outCelsius = static_cast<float>(_thermistor->readCelsius());
    return true;
}
