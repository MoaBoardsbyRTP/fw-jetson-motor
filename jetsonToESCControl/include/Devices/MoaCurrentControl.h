/**
 * @file MoaCurrentControl.h
 * @brief Current monitoring class with threshold detection for ESP32
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * This library provides current monitoring via ADC using a Hall effect sensor
 * (ACS759-200B) that integrates with the Moa event queue system. When current
 * crosses thresholds, it pushes events to the specified FreeRTOS queue for
 * processing by ControlTask.
 * 
 * ## ACS759-200B Specifications
 * - Sensitivity: 6.6 mV/A
 * - Output at 0A: VCC/2 (1.65V at 3.3V supply)
 * - Range: ±200A (bidirectional)
 * - Output voltage: 0V to 3.3V
 */

#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ControlCommand.h"
#include "StatsReading.h"

/**
 * @brief Default number of samples for current averaging
 */
#define MOA_CURRENT_DEFAULT_SAMPLES 10

/**
 * @brief Maximum number of samples for current averaging
 */
#define MOA_CURRENT_MAX_SAMPLES 32

/**
 * @brief Current state enumeration
 */
enum class MoaCurrentState {
    NORMAL,              ///< Current within normal operating range
    OVERCURRENT,         ///< Current above positive threshold
    REVERSE_OVERCURRENT  ///< Current below negative threshold (reverse flow)
};

/**
 * @brief Current monitoring class with threshold-based events and averaging
 * 
 * MoaCurrentControl provides current monitoring using a Hall effect sensor
 * (ACS759-200B or similar) with:
 * - Configurable sensitivity for different sensor models
 * - Configurable zero-current offset voltage
 * - Configurable moving average filtering
 * - Bidirectional current detection (positive and negative)
 * - Threshold detection with hysteresis
 * - Event-driven integration via FreeRTOS queue
 * 
 * When current crosses thresholds, it automatically pushes a ControlCommand
 * event to the configured queue.
 * 
 * @note Events are pushed when current transitions between states:
 *       - COMMAND_CURRENT_OVERCURRENT: Current exceeded positive threshold
 *       - COMMAND_CURRENT_NORMAL: Current returned to normal range
 *       - COMMAND_CURRENT_REVERSE_OVERCURRENT: Current exceeded negative threshold
 * 
 * ## ACS759-200B Configuration
 * - Sensitivity: 6.6 mV/A
 * - Zero offset: VCC/2 = 1.65V (at 3.3V supply)
 * - Formula: Current = (Vadc - Voffset) / Sensitivity
 * 
 * ## Usage Example
 * @code
 * QueueHandle_t eventQueue = xQueueCreate(10, sizeof(ControlCommand));
 * MoaCurrentControl currentSensor(eventQueue, ADC_PIN);
 * 
 * currentSensor.setSensitivity(0.0066f);     // 6.6 mV/A = 0.0066 V/A
 * currentSensor.setZeroOffset(1.65f);        // VCC/2 at 0A
 * currentSensor.setReferenceVoltage(3.3f);   // ESP32 ADC reference
 * currentSensor.setOvercurrentThreshold(150.0f);  // 150A overcurrent
 * currentSensor.setHysteresis(5.0f);         // 5A hysteresis
 * currentSensor.begin();
 * 
 * // In SensorTask, call periodically:
 * currentSensor.update();
 * 
 * // In ControlTask, handle the event:
 * // if (cmd.controlType == CONTROL_TYPE_CURRENT) {
 * //     float current = cmd.value / 10.0f;  // Current in A (x10 for precision)
 * //     if (cmd.commandType == COMMAND_CURRENT_OVERCURRENT) {
 * //         stateMachine.overcurrentDetected(cmd);
 * //     }
 * // }
 * @endcode
 */
class MoaCurrentControl {
public:
    /**
     * @brief Construct a new MoaCurrentControl object
     * 
     * @param eventQueue FreeRTOS queue handle to push current events to
     * @param adcPin ADC pin connected to the Hall effect sensor output
     * @param numSamples Number of samples for moving average (default: MOA_CURRENT_DEFAULT_SAMPLES)
     */
    MoaCurrentControl(QueueHandle_t eventQueue, uint8_t adcPin,
                      uint8_t numSamples = MOA_CURRENT_DEFAULT_SAMPLES);

    /**
     * @brief Destructor - frees allocated sample buffer
     */
    ~MoaCurrentControl();

    /**
     * @brief Initialize the ADC for current monitoring
     * @note Must be called before update()
     */
    void begin();

    /**
     * @brief Read ADC, update average, and check thresholds
     * 
     * This method:
     * 1. Reads the ADC value from the configured pin
     * 2. Converts to current using sensitivity and offset
     * 3. Updates the moving average
     * 4. Checks if current crossed thresholds and pushes event if needed
     * 
     * @note Should be called periodically (e.g., from SensorTask)
     */
    void update();

    /**
     * @brief Set the sensor sensitivity
     * 
     * @param sensitivity Sensitivity in V/A (e.g., 0.0066 for ACS759-200B)
     */
    void setSensitivity(float sensitivity);

    /**
     * @brief Get the current sensor sensitivity
     * @return float Sensitivity in V/A
     */
    float getSensitivity() const;

    /**
     * @brief Set the zero-current offset voltage
     * 
     * This is the voltage output by the sensor when no current flows.
     * For ratiometric sensors, this is typically VCC/2.
     * 
     * @param offset Zero offset voltage in volts (e.g., 1.65V for 3.3V supply)
     */
    void setZeroOffset(float offset);

    /**
     * @brief Get the zero-current offset voltage
     * @return float Zero offset in volts
     */
    float getZeroOffset() const;

    /**
     * @brief Set the ADC reference voltage
     * 
     * @param voltage Reference voltage in volts (typically 3.3V for ESP32)
     */
    void setReferenceVoltage(float voltage);

    /**
     * @brief Get the ADC reference voltage
     * @return float Reference voltage in volts
     */
    float getReferenceVoltage() const;

    /**
     * @brief Set the overcurrent threshold (positive direction)
     * 
     * When current exceeds this threshold, COMMAND_CURRENT_OVERCURRENT is sent.
     * 
     * @param current Threshold in Amps (positive value)
     */
    void setOvercurrentThreshold(float current);

    /**
     * @brief Get the overcurrent threshold
     * @return float Overcurrent threshold in Amps
     */
    float getOvercurrentThreshold() const;

    /**
     * @brief Set the reverse overcurrent threshold (negative direction)
     * 
     * When current drops below this threshold (more negative),
     * COMMAND_CURRENT_REVERSE_OVERCURRENT is sent.
     * 
     * @param current Threshold in Amps (negative value, e.g., -150.0)
     */
    void setReverseOvercurrentThreshold(float current);

    /**
     * @brief Get the reverse overcurrent threshold
     * @return float Reverse overcurrent threshold in Amps
     */
    float getReverseOvercurrentThreshold() const;

    /**
     * @brief Set the hysteresis value for threshold detection
     * 
     * Prevents rapid toggling when current hovers near a threshold.
     * 
     * @param hysteresis Hysteresis value in Amps
     */
    void setHysteresis(float hysteresis);

    /**
     * @brief Get the hysteresis value
     * @return float Hysteresis in Amps
     */
    float getHysteresis() const;

    /**
     * @brief Get the current raw ADC reading
     * @return uint16_t Raw ADC value (0-4095 for 12-bit)
     */
    uint16_t getRawAdc() const;

    /**
     * @brief Get the current ADC voltage (not converted to current)
     * @return float ADC voltage in volts
     */
    float getAdcVoltage() const;

    /**
     * @brief Get the current calculated current (not averaged)
     * @return float Current in Amps
     */
    float getCurrentReading() const;

    /**
     * @brief Get the current averaged current
     * @return float Averaged current in Amps
     */
    float getAveragedCurrent() const;

    /**
     * @brief Get the current state
     * @return MoaCurrentState Current state (NORMAL, OVERCURRENT, or REVERSE_OVERCURRENT)
     */
    MoaCurrentState getState() const;

    /**
     * @brief Check if the sample buffer is full (averaging is valid)
     * @return true if buffer has been filled at least once
     */
    bool isAveragingReady() const;

    /**
     * @brief Set the number of samples for averaging
     * @param numSamples Number of samples (1 to MOA_CURRENT_MAX_SAMPLES)
     * @note This resets the sample buffer
     */
    void setNumSamples(uint8_t numSamples);

    /**
     * @brief Get the number of samples used for averaging
     * @return uint8_t Number of samples
     */
    uint8_t getNumSamples() const;

    /**
     * @brief Set the ADC resolution in bits
     * @param bits ADC resolution (default 12 for ESP32)
     */
    void setAdcResolution(uint8_t bits);

    /**
     * @brief Get the ADC resolution
     * @return uint8_t ADC resolution in bits
     */
    uint8_t getAdcResolution() const;

    /**
     * @brief Set the event queue handle (must be called after queue creation)
     * @param eventQueue FreeRTOS queue handle for control events
     */
    void setEventQueue(QueueHandle_t eventQueue);

    /**
     * @brief Set the stats queue for telemetry
     * @param statsQueue FreeRTOS queue handle for stats readings
     */
    void setStatsQueue(QueueHandle_t statsQueue);

private:
    QueueHandle_t _eventQueue;         ///< Queue to push events to
    QueueHandle_t _statsQueue;         ///< Queue to push stats readings to
    uint8_t _adcPin;                   ///< ADC pin number
    uint8_t _adcResolution;            ///< ADC resolution in bits
    float _sensitivity;                ///< Sensor sensitivity in V/A
    float _zeroOffset;                 ///< Zero-current offset voltage
    float _referenceVoltage;           ///< ADC reference voltage
    float _overcurrentThreshold;       ///< Positive overcurrent threshold
    float _reverseOvercurrentThreshold;///< Negative overcurrent threshold
    float _hysteresis;                 ///< Hysteresis for threshold detection
    uint16_t _rawAdc;                  ///< Current raw ADC reading
    float _adcVoltage;                 ///< Current ADC voltage
    float _currentReading;             ///< Current calculated current
    MoaCurrentState _state;            ///< Current state

    float* _samples;                   ///< Circular buffer for current samples
    uint8_t _numSamples;               ///< Number of samples for averaging
    uint8_t _sampleIndex;              ///< Current index in circular buffer
    uint8_t _sampleCount;              ///< Number of valid samples in buffer
    float _averagedCurrent;            ///< Cached averaged current
    uint32_t _updateCount;             ///< Counter for periodic logging

    /**
     * @brief Add a new sample to the circular buffer and update average
     * @param current Current value to add
     */
    void addSample(float current);

    /**
     * @brief Calculate the average of all samples in the buffer
     * @return float Averaged current
     */
    float calculateAverage() const;

    /**
     * @brief Convert raw ADC value to current in Amps
     * @param rawAdc Raw ADC reading
     * @return float Current in Amps
     */
    float adcToCurrent(uint16_t rawAdc);

    /**
     * @brief Push a current event to the queue
     * @param commandType The command type (COMMAND_CURRENT_*)
     */
    void pushCurrentEvent(int commandType);

    /**
     * @brief Push a stats reading to the stats queue
     */
    void pushStatsReading();
};
