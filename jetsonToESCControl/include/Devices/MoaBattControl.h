/**
 * @file MoaBattControl.h
 * @brief Battery level monitoring class with threshold detection for ESP32
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * This library provides battery voltage monitoring via ADC with voltage divider
 * support that integrates with the Moa event queue system. When battery level
 * crosses thresholds, it pushes events to the specified FreeRTOS queue for
 * processing by ControlTask.
 */

#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ControlCommand.h"
#include "StatsReading.h"

/**
 * @brief Default number of samples for battery voltage averaging
 */
#define MOA_BATT_DEFAULT_SAMPLES 10

/**
 * @brief Maximum number of samples for battery voltage averaging
 */
#define MOA_BATT_MAX_SAMPLES 32

/**
 * @brief Control type identifier for battery events
 */
#define CONTROL_TYPE_BATTERY 102

/**
 * @brief Command type for battery level entered HIGH zone
 */
#define COMMAND_BATT_LEVEL_HIGH 1

/**
 * @brief Command type for battery level entered MEDIUM zone
 */
#define COMMAND_BATT_LEVEL_MEDIUM 2

/**
 * @brief Command type for battery level entered LOW zone
 */
#define COMMAND_BATT_LEVEL_LOW 3

/**
 * @brief Battery level state enumeration
 */
enum class MoaBattLevel {
    BATT_LOW,     ///< Battery below low threshold (critical)
    BATT_MEDIUM,  ///< Battery between low and high thresholds
    BATT_HIGH     ///< Battery above high threshold (fully charged)
};

/**
 * @brief Battery monitoring class with threshold-based events and averaging
 * 
 * MoaBattControl provides battery voltage monitoring via ADC with:
 * - Configurable voltage divider ratio for accurate voltage calculation
 * - Configurable moving average filtering
 * - Two-threshold detection (low and high) creating three zones
 * - Event-driven integration via FreeRTOS queue
 * 
 * When battery level crosses thresholds, it automatically pushes a ControlCommand
 * event to the configured queue.
 * 
 * @note Events are pushed when battery level transitions between zones:
 *       - COMMAND_BATT_LEVEL_HIGH: Entered HIGH zone (above high threshold)
 *       - COMMAND_BATT_LEVEL_MEDIUM: Entered MEDIUM zone (between thresholds)
 *       - COMMAND_BATT_LEVEL_LOW: Entered LOW zone (below low threshold)
 * 
 * ## Voltage Divider Configuration
 * For a voltage divider with R1 (top) and R2 (bottom):
 * - dividerRatio = (R1 + R2) / R2
 * - Example: R1=100k, R2=47k → ratio = 3.128
 * 
 * ## Usage Example
 * @code
 * QueueHandle_t eventQueue = xQueueCreate(10, sizeof(ControlCommand));
 * MoaBattControl battery(eventQueue, ADC_PIN);
 * 
 * battery.setDividerRatio(3.128f);      // 100k/47k divider
 * battery.setReferenceVoltage(3.3f);    // ESP32 ADC reference
 * battery.setLowThreshold(3.3f);        // 3.3V = low battery
 * battery.setHighThreshold(4.0f);       // 4.0V = high battery
 * battery.begin();
 * 
 * // In SensorTask, call periodically:
 * battery.update();
 * 
 * // In ControlTask, handle the event:
 * // if (cmd.controlType == CONTROL_TYPE_BATTERY) {
 * //     float voltage = cmd.value / 1000.0f;  // Voltage in V (mV for precision)
 * //     stateMachine.batteryLevelCrossedLimit(cmd);
 * // }
 * @endcode
 */
class MoaBattControl {
public:
    /**
     * @brief Construct a new MoaBattControl object
     * 
     * @param eventQueue FreeRTOS queue handle to push battery events to
     * @param adcPin ADC pin connected to the voltage divider output
     * @param numSamples Number of samples for moving average (default: MOA_BATT_DEFAULT_SAMPLES)
     */
    MoaBattControl(QueueHandle_t eventQueue, uint8_t adcPin,
                   uint8_t numSamples = MOA_BATT_DEFAULT_SAMPLES);

    /**
     * @brief Destructor - frees allocated sample buffer
     */
    ~MoaBattControl();

    /**
     * @brief Initialize the ADC for battery monitoring
     * @note Must be called before update()
     */
    void begin();

    /**
     * @brief Read ADC, update average, and check thresholds
     * 
     * This method:
     * 1. Reads the ADC value from the configured pin
     * 2. Converts to voltage using divider ratio
     * 3. Updates the moving average
     * 4. Checks if voltage crossed thresholds and pushes event if needed
     * 
     * @note Should be called periodically (e.g., from SensorTask)
     */
    void update();

    /**
     * @brief Set the voltage divider ratio
     * 
     * For a divider with R1 (top, connected to battery) and R2 (bottom, to GND):
     * ratio = (R1 + R2) / R2
     * 
     * @param ratio Voltage divider ratio (must be >= 1.0)
     */
    void setDividerRatio(float ratio);

    /**
     * @brief Get the current voltage divider ratio
     * @return float Divider ratio
     */
    float getDividerRatio() const;

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
     * @brief Set the low battery threshold voltage
     * 
     * When battery voltage drops below this threshold, COMMAND_BATT_LEVEL_LOW is sent.
     * 
     * @param voltage Low threshold in volts (actual battery voltage, not divided)
     */
    void setLowThreshold(float voltage);

    /**
     * @brief Get the low battery threshold
     * @return float Low threshold in volts
     */
    float getLowThreshold() const;

    /**
     * @brief Set the high battery threshold voltage
     * 
     * When battery voltage rises above this threshold, COMMAND_BATT_LEVEL_HIGH is sent.
     * 
     * @param voltage High threshold in volts (actual battery voltage, not divided)
     */
    void setHighThreshold(float voltage);

    /**
     * @brief Get the high battery threshold
     * @return float High threshold in volts
     */
    float getHighThreshold() const;

    /**
     * @brief Set the hysteresis value for threshold detection
     * 
     * Prevents rapid toggling when voltage hovers near a threshold.
     * 
     * @param hysteresis Hysteresis value in volts (applied to both thresholds)
     */
    void setHysteresis(float hysteresis);

    /**
     * @brief Get the hysteresis value
     * @return float Hysteresis in volts
     */
    float getHysteresis() const;

    /**
     * @brief Get the current raw ADC reading
     * @return uint16_t Raw ADC value (0-4095 for 12-bit)
     */
    uint16_t getRawAdc() const;

    /**
     * @brief Get the current calculated battery voltage (not averaged)
     * @return float Battery voltage in volts
     */
    float getCurrentVoltage() const;

    /**
     * @brief Get the current averaged battery voltage
     * @return float Averaged battery voltage in volts
     */
    float getAveragedVoltage() const;

    /**
     * @brief Get the current battery level state
     * @return MoaBattLevel Current level (LOW, MEDIUM, or HIGH)
     */
    MoaBattLevel getLevel() const;

    /**
     * @brief Check if the sample buffer is full (averaging is valid)
     * @return true if buffer has been filled at least once
     */
    bool isAveragingReady() const;

    /**
     * @brief Set the number of samples for averaging
     * @param numSamples Number of samples (1 to MOA_BATT_MAX_SAMPLES)
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
     * @brief Set the stats queue for telemetry
     * @param statsQueue FreeRTOS queue handle for stats readings
     */
    void setStatsQueue(QueueHandle_t statsQueue);

private:
    QueueHandle_t _eventQueue;         ///< Queue to push events to
    QueueHandle_t _statsQueue;         ///< Queue to push stats readings to
    uint8_t _adcPin;                   ///< ADC pin number
    uint8_t _adcResolution;            ///< ADC resolution in bits
    float _dividerRatio;               ///< Voltage divider ratio
    float _referenceVoltage;           ///< ADC reference voltage
    float _lowThreshold;               ///< Low battery threshold voltage
    float _highThreshold;              ///< High battery threshold voltage
    float _hysteresis;                 ///< Hysteresis for threshold detection
    uint16_t _rawAdc;                  ///< Current raw ADC reading
    float _currentVoltage;             ///< Current calculated voltage
    MoaBattLevel _level;               ///< Current battery level state

    float* _samples;                   ///< Circular buffer for voltage samples
    uint8_t _numSamples;               ///< Number of samples for averaging
    uint8_t _sampleIndex;              ///< Current index in circular buffer
    uint8_t _sampleCount;              ///< Number of valid samples in buffer
    float _averagedVoltage;            ///< Cached averaged voltage

    /**
     * @brief Add a new sample to the circular buffer and update average
     * @param voltage Voltage value to add
     */
    void addSample(float voltage);

    /**
     * @brief Calculate the average of all samples in the buffer
     * @return float Averaged voltage
     */
    float calculateAverage() const;

    /**
     * @brief Convert raw ADC value to actual battery voltage
     * @param rawAdc Raw ADC reading
     * @return float Battery voltage in volts
     */
    float adcToVoltage(uint16_t rawAdc) const;

    /**
     * @brief Push a battery level event to the queue
     * @param commandType The command type (COMMAND_BATT_LEVEL_*)
     */
    void pushBattEvent(int commandType);

    /**
     * @brief Push a stats reading to the stats queue
     */
    void pushStatsReading();
};
