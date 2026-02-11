/*!
* @file ESCController.h
* @brief ESCController class for controlling ESCs
* @author Oscar Martinez
* @date 2025-01-28
*/

#pragma once

#include "Arduino.h"
#include "Constants.h"

class ESCController{
public:
    /**
     * @brief Construct a new ESCController object
     * @param pin GPIO pin connected to the ESC signal line
     * @param channel LEDC channel to use for PWM generation (0-15)
     * @param frequency PWM frequency in Hz (default 50Hz for standard ESCs)
     */
    ESCController(uint8_t pin, uint8_t channel, uint16_t frequency = 50);

    /**
     * @brief Initialize the ESC controller, sets up PWM on the configured pin
     */
    void begin();

    /**
     * @brief Write the current throttle value to the ESC via PWM
     */
    void writeThrottle();

    /**
     * @brief Set the throttle value immediately (clamped to min/max bounds)
     * @param throttle Throttle value (0-1023 for 10-bit resolution)
     */
    void setThrottle(uint16_t throttle);

    /**
     * @brief Configure a ramped throttle transition
     * @param rampTime Number of updateRamp() calls to reach target
     * @param targetThrottle Target throttle value to ramp towards
     */
    void setRampThrottle(uint16_t rampTime, uint16_t targetThrottle);

    /**
     * @brief Update the ramp state, call this periodically from a timer/task
     * @note Safe to call when not ramping - will return immediately
     */
    void updateThrottle();

    /**
     * @brief Check if a ramp transition is currently in progress
     * @return true if ramping, false otherwise
     */
    bool isRamping() const;

    /**
     * @brief Set throttle by percentage with ramped transition
     * @param percent Throttle percentage (0-100)
     */
    void setThrottlePercent(uint8_t percent);

    /**
     * @brief Set the ramp rate
     * @param ratePercentPerSec Ramp rate in %/s
     */
    void setRampRate(float ratePercentPerSec);

    /**
     * @brief Set the tick period used for ramp step calculation
     * @param periodMs Tick interval in milliseconds
     */
    void setTickPeriod(uint16_t periodMs);

    /**
     * @brief Emergency stop - immediately sets throttle to minimum
     */
    void stop();

    /**
     * @brief Get the current throttle duty cycle value
     * @return uint16_t Current throttle (0-1023)
     */
    uint16_t getCurrentThrottle() const;
private:
    uint8_t _pin;
    uint8_t _channel;
    uint16_t _frequency;
    uint8_t _resolution;
    uint16_t _throttle;
    uint16_t _minThrottle;
    uint16_t _maxThrottle;
    uint16_t _currentThrottle;
    uint16_t _targetThrottle;
    uint16_t _rampTime;
    int16_t _rampStep;
    bool _ramping;
    float _rampRate;        // %/s
    uint16_t _tickPeriodMs; // ms per updateThrottle() call
};