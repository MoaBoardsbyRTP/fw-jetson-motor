/**
 * @file MoaTempControl.h
 * @brief Temperature control class with hysteresis and averaging for ESP32
 * @author Oscar Martinez
 * @date 2025-01-28
 * 
 * This library provides temperature monitoring using Dallas DS18B20 sensors
 * that integrates with the Moa event queue system. When temperature crosses
 * thresholds, it pushes an event to the specified FreeRTOS queue for processing
 * by ControlTask.
 */

#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ControlCommand.h"
#include "StatsReading.h"

/**
 * @brief Default number of samples for temperature averaging
 */
#define MOA_TEMP_DEFAULT_SAMPLES 10

/**
 * @brief Maximum number of samples for temperature averaging
 */
#define MOA_TEMP_MAX_SAMPLES 32

/**
 * @brief Control type identifier for temperature events
 */
#define CONTROL_TYPE_TEMPERATURE 101

/**
 * @brief Command type for temperature crossed above target
 */
#define COMMAND_TEMP_CROSSED_ABOVE 1

/**
 * @brief Command type for temperature crossed below threshold (target - hysteresis)
 */
#define COMMAND_TEMP_CROSSED_BELOW 2

/**
 * @brief Temperature state enumeration for hysteresis control
 */
enum class MoaTempState {
    BELOW_TARGET,  ///< Temperature is below (target - hysteresis)
    ABOVE_TARGET   ///< Temperature is above target
};

/**
 * @brief Temperature control class with hysteresis-based events and averaging
 * 
 * MoaTempControl provides temperature monitoring using Dallas DS18B20 sensors with:
 * - Configurable moving average filtering
 * - Hysteresis-based threshold detection
 * - Event-driven integration via FreeRTOS queue
 * 
 * When temperature crosses thresholds, it automatically pushes a ControlCommand
 * event to the configured queue.
 * 
 * @note Events are pushed when:
 *       - Temperature crosses UP above the target temperature (COMMAND_TEMP_CROSSED_ABOVE)
 *       - Temperature crosses DOWN below (target - hysteresis) (COMMAND_TEMP_CROSSED_BELOW)
 * 
 * ## Usage Example
 * @code
 * QueueHandle_t eventQueue = xQueueCreate(10, sizeof(ControlCommand));
 * MoaTempControl tempSensor(eventQueue, TEMP_SENSOR_PIN);
 * 
 * tempSensor.setTargetTemp(50.0f);
 * tempSensor.setHysteresis(5.0f);  // Lower threshold = 45°C
 * tempSensor.begin();
 * 
 * // In SensorTask, call periodically:
 * tempSensor.update();
 * 
 * // In ControlTask, handle the event:
 * // if (cmd.controlType == CONTROL_TYPE_TEMPERATURE) {
 * //     float temp = cmd.value / 10.0f;  // Temperature in °C (x10 for precision)
 * //     stateMachine.temperatureCrossedLimit(cmd);
 * // }
 * @endcode
 */
class MoaTempControl {
public:
    /**
     * @brief Construct a new MoaTempControl object
     * 
     * @param eventQueue FreeRTOS queue handle to push temperature events to
     * @param pin GPIO pin connected to the DS18B20 data line
     * @param numSamples Number of samples for moving average (default: MOA_TEMP_DEFAULT_SAMPLES)
     */
    MoaTempControl(QueueHandle_t eventQueue, uint8_t pin,
                   uint8_t numSamples = MOA_TEMP_DEFAULT_SAMPLES);

    /**
     * @brief Destructor - frees allocated sample buffer
     */
    ~MoaTempControl();

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
     * 3. Checks if temperature crossed thresholds and pushes event if needed
     * 
     * @note Should be called periodically (e.g., from SensorTask)
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
     * Event fires when crossing down below this threshold.
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
     * @return MoaTempState Current state (BELOW_TARGET or ABOVE_TARGET)
     */
    MoaTempState getState() const;

    /**
     * @brief Check if the sample buffer is full (averaging is valid)
     * @return true if buffer has been filled at least once
     */
    bool isAveragingReady() const;

    /**
     * @brief Set the number of samples for averaging
     * @param numSamples Number of samples (1 to MOA_TEMP_MAX_SAMPLES)
     * @note This resets the sample buffer
     */
    void setNumSamples(uint8_t numSamples);

    /**
     * @brief Get the number of samples used for averaging
     * @return uint8_t Number of samples
     */
    uint8_t getNumSamples() const;

    /**
     * @brief Set the stats queue for telemetry
     * @param statsQueue FreeRTOS queue handle for stats readings
     */
    void setStatsQueue(QueueHandle_t statsQueue);

private:
    QueueHandle_t _eventQueue;             ///< Queue to push events to
    QueueHandle_t _statsQueue;             ///< Queue to push stats readings to
    OneWire _oneWire;                      ///< OneWire bus instance
    DallasTemperature _sensors;            ///< Dallas temperature sensor interface
    float _targetTemp;                     ///< Target temperature threshold
    float _currentTemp;                    ///< Current raw temperature reading
    float _hysteresis;                     ///< Hysteresis value for lower threshold
    MoaTempState _state;                   ///< Current temperature state
    
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

    /**
     * @brief Push a temperature event to the queue
     * 
     * @param commandType The command type (COMMAND_TEMP_CROSSED_ABOVE or COMMAND_TEMP_CROSSED_BELOW)
     */
    void pushTempEvent(int commandType);

    /**
     * @brief Push a stats reading to the stats queue
     */
    void pushStatsReading();
};