/**
 * @file MoaTimer.h
 * @brief FreeRTOS-based timer wrapper for event-driven architecture
 * @author Oscar Martinez
 * @date 2025-01-29
 * 
 * This library provides a thin wrapper around FreeRTOS software timers (xTimer)
 * that integrates with the Moa event queue system. When a timer expires, it
 * pushes an event to the specified FreeRTOS queue for processing by ControlTask.
 */

#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "ControlCommand.h"

/**
 * @brief Control type identifier for timer events
 */
#define CONTROL_TYPE_TIMER 100

/**
 * @brief Maximum number of MoaTimer instances supported
 * @note Used for static callback routing
 */
#define MOA_TIMER_MAX_INSTANCES 8

/**
 * @brief FreeRTOS software timer wrapper for event-driven systems
 * 
 * MoaTimer wraps FreeRTOS xTimer functionality and integrates with the
 * Moa event queue. When a timer expires, it automatically pushes a
 * ControlCommand event to the configured queue.
 * 
 * @note Timer callbacks execute in the FreeRTOS timer service task context.
 *       The callback only pushes an event to the queue; actual handling
 *       should occur in ControlTask to maintain thread safety.
 * 
 * @note The timer ID is sent in cmd.commandType to distinguish between multiple
 *       timer instances. cmd.value is available for future use (e.g., elapsed time).
 * 
 * ## Usage Example
 * @code
 * QueueHandle_t eventQueue = xQueueCreate(10, sizeof(ControlCommand));
 * MoaTimer fullThrottleTimer(eventQueue, TIMER_ID_FULL_THROTTLE);
 * MoaTimer cooldownTimer(eventQueue, TIMER_ID_COOLDOWN);
 * 
 * // Start a 5-second one-shot timer
 * fullThrottleTimer.start(5000);
 * 
 * // In ControlTask, handle the event:
 * // if (cmd.controlType == CONTROL_TYPE_TIMER) {
 * //     if (cmd.commandType == TIMER_ID_FULL_THROTTLE) {
 * //         // Full throttle timer expired
 * //     } else if (cmd.commandType == TIMER_ID_COOLDOWN) {
 * //         // Cooldown timer expired
 * //     }
 * // }
 * @endcode
 */
class MoaTimer {
public:
    /**
     * @brief Construct a new MoaTimer object
     * 
     * @param eventQueue FreeRTOS queue handle to push timer events to
     * @param timerId Unique identifier for this timer (used in event commandType field)
     * @param name Optional name for debugging (max 16 chars, default: "MoaTimer")
     * 
     * @note The timer is created but not started. Call start() to begin timing.
     * @note timerId should be unique per timer instance to distinguish events.
     */
    MoaTimer(QueueHandle_t eventQueue, uint8_t timerId, const char* name = "MoaTimer");

    /**
     * @brief Destroy the MoaTimer object
     * 
     * Stops and deletes the underlying FreeRTOS timer.
     */
    ~MoaTimer();

    /**
     * @brief Start or restart the timer
     * 
     * @param durationMs Timer duration in milliseconds
     * @param autoReload If true, timer repeats automatically; if false, one-shot (default)
     * @return true if timer started successfully
     * @return false if timer failed to start (e.g., timer service queue full)
     * 
     * @note If the timer is already running, it will be stopped and restarted
     *       with the new duration.
     * @note For auto-reload timers, the callback fires repeatedly until stop() is called.
     */
    bool start(uint32_t durationMs, bool autoReload = false);

    /**
     * @brief Stop the timer
     * 
     * @return true if timer stopped successfully
     * @return false if timer failed to stop
     * 
     * @note Safe to call even if timer is not running.
     */
    bool stop();

    /**
     * @brief Reset the timer to its original duration
     * 
     * @return true if timer reset successfully
     * @return false if timer failed to reset
     * 
     * @note If the timer is not running, this will start it.
     */
    bool reset();

    /**
     * @brief Check if the timer is currently running
     * 
     * @return true if timer is active
     * @return false if timer is stopped or expired
     */
    bool isRunning() const;

    /**
     * @brief Get the timer's unique identifier
     * 
     * @return uint8_t Timer ID (as set in constructor)
     */
    uint8_t getTimerId() const;

    /**
     * @brief Get the configured duration
     * 
     * @return uint32_t Duration in milliseconds
     */
    uint32_t getDuration() const;

    /**
     * @brief Change the timer duration without restarting
     * 
     * @param durationMs New duration in milliseconds
     * @return true if period changed successfully
     * @return false if change failed
     * 
     * @note Takes effect on next start/reset. Does not affect currently running timer.
     */
    bool setDuration(uint32_t durationMs);

private:
    TimerHandle_t _timerHandle;     ///< FreeRTOS timer handle
    QueueHandle_t _eventQueue;      ///< Queue to push events to
    uint8_t _timerId;               ///< Unique timer identifier
    uint32_t _durationMs;           ///< Timer duration in milliseconds
    bool _autoReload;               ///< Auto-reload flag

    /**
     * @brief Static callback function for FreeRTOS timer
     * 
     * @param xTimer Timer handle that expired
     * 
     * @note This function retrieves the MoaTimer instance from the timer ID
     *       and pushes a ControlCommand event to the configured queue.
     */
    static void timerCallback(TimerHandle_t xTimer);

    /**
     * @brief Push a timer expired event to the queue
     * 
     * Called by the static callback to push the event.
     */
    void pushTimerEvent();
};
