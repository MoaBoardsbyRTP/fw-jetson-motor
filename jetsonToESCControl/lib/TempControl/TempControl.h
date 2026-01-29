/**
 * @file TempControl.h
 * @brief Temperature control class with hysteresis and averaging for ESP32
 * @author Oscar Martinez
 * @date 2025-01-28
 */

#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/**
 * @brief Default number of samples for temperature averaging
 */
#define TEMP_CONTROL_DEFAULT_SAMPLES 10

/**
 * @brief Maximum number of samples for temperature averaging
 */
#define TEMP_CONTROL_MAX_SAMPLES 32

/**
 * @brief Temperature state enumeration for hysteresis control
 */
enum class TempState {
    BELOW_TARGET,  ///< Temperature is below (target - hysteresis)
    ABOVE_TARGET   ///< Temperature is above target
};

/**
 * @brief Temperature control class with hysteresis-based callbacks and averaging
 * 
 * This class provides temperature monitoring using Dallas DS18B20 sensors with:
 * - Configurable moving average filtering
 * - Hysteresis-based threshold detection
 * - Callbacks fired when temperature crosses target (up) or target-hysteresis (down)
 * 
 * @note The callback is fired when:
 *       - Temperature crosses UP above the target temperature
 *       - Temperature crosses DOWN below (target - hysteresis)
 */
class TempControl {
public:
    /**
     * @brief Construct a new TempControl object
     * @param pin GPIO pin connected to the DS18B20 data line
     * @param numSamples Number of samples for moving average (default: TEMP_CONTROL_DEFAULT_SAMPLES)
     */
    TempControl(uint8_t pin, uint8_t numSamples = TEMP_CONTROL_DEFAULT_SAMPLES);

    /**
     * @brief Destructor - frees allocated sample buffer
     */
    ~TempControl();

    /**
     * @brief Initialize the temperature sensor
     * @note Must be called before update()
     */
    void begin();

    /**
     * @brief Read temperature, update average, and check thresholds
     * 
     * This method:
     * 1. Requests a new temperature reading from the sensor
     * 2. Updates the moving average
     * 3. Checks if temperature crossed thresholds and fires callback if needed
     * 
     * @note Should be called periodically (e.g., in loop())
     */
    void update();

    /**
     * @brief Set the target temperature threshold
     * @param temp Target temperature in Celsius
     */
    void setTargetTemp(float temp);

    /**
     * @brief Get the current target temperature
     * @return float Target temperature in Celsius
     */
    float getTargetTemp() const;

    /**
     * @brief Set the hysteresis value
     * 
     * The hysteresis defines the lower threshold as (target - hysteresis).
     * Callback fires when crossing down below this threshold.
     * 
     * @param hysteresis Hysteresis value in Celsius (must be positive)
     */
    void setHysteresis(float hysteresis);

    /**
     * @brief Get the current hysteresis value
     * @return float Hysteresis value in Celsius
     */
    float getHysteresis() const;

    /**
     * @brief Set the callback function for temperature threshold events
     * 
     * The callback receives the current averaged temperature when:
     * - Temperature crosses UP above target
     * - Temperature crosses DOWN below (target - hysteresis)
     * 
     * @param callback Function pointer with signature void(float temperature, bool isAboveTarget)
     *                 - temperature: Current averaged temperature in Celsius
     *                 - isAboveTarget: true if crossed above target, false if crossed below threshold
     */
    void setCallback(void (*callback)(float, bool));

    /**
     * @brief Get the current raw temperature reading
     * @return float Current temperature in Celsius (not averaged)
     */
    float getCurrentTemp() const;

    /**
     * @brief Get the current averaged temperature
     * @return float Averaged temperature in Celsius
     */
    float getAveragedTemp() const;

    /**
     * @brief Get the current temperature state
     * @return TempState Current state (BELOW_TARGET or ABOVE_TARGET)
     */
    TempState getState() const;

    /**
     * @brief Check if the sample buffer is full (averaging is valid)
     * @return true if buffer has been filled at least once
     */
    bool isAveragingReady() const;

    /**
     * @brief Set the number of samples for averaging
     * @param numSamples Number of samples (1 to TEMP_CONTROL_MAX_SAMPLES)
     * @note This resets the sample buffer
     */
    void setNumSamples(uint8_t numSamples);

    /**
     * @brief Get the number of samples used for averaging
     * @return uint8_t Number of samples
     */
    uint8_t getNumSamples() const;

private:
    OneWire _oneWire;                      ///< OneWire bus instance
    DallasTemperature _sensors;            ///< Dallas temperature sensor interface
    float _targetTemp;                     ///< Target temperature threshold
    float _currentTemp;                    ///< Current raw temperature reading
    float _hysteresis;                     ///< Hysteresis value for lower threshold
    void (*_callback)(float, bool);        ///< Callback function pointer
    TempState _state;                      ///< Current temperature state
    
    float* _samples;                       ///< Circular buffer for temperature samples
    uint8_t _numSamples;                   ///< Number of samples for averaging
    uint8_t _sampleIndex;                  ///< Current index in circular buffer
    uint8_t _sampleCount;                  ///< Number of valid samples in buffer
    float _averagedTemp;                   ///< Cached averaged temperature

    /**
     * @brief Add a new sample to the circular buffer and update average
     * @param temp Temperature value to add
     */
    void addSample(float temp);

    /**
     * @brief Calculate the average of all samples in the buffer
     * @return float Averaged temperature
     */
    float calculateAverage() const;
};