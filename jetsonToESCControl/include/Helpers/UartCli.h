/**
 * @file UartCli.h
 * @brief UART command-line interface for runtime configuration
 * @author Oscar Martinez
 * @date 2026-02-15
 * 
 * Provides a serial command parser for reading and writing ConfigManager
 * settings at runtime. Designed for fine-tuning during development and
 * as a debug interface in production.
 * 
 * Protocol: newline-terminated ASCII commands over Serial (115200 baud).
 * See UART_CLI.md for full command reference.
 */

#pragma once

#include <Arduino.h>
#include "ConfigManager.h"

// Forward declarations
class MoaBattControl;
class MoaCurrentControl;
class MoaTempControl;
class ESCController;

/**
 * @brief Maximum input line length
 */
#define UART_CLI_MAX_LINE 128

/**
 * @brief UART command-line interface
 * 
 * Call begin() once, then poll() from a FreeRTOS task or loop.
 * Each poll() reads available serial bytes and processes complete lines.
 */
class UartCli {
public:
    /**
     * @brief Construct a new UartCli
     * @param config Reference to ConfigManager
     * @param batt Reference to battery control (for hot-reload)
     * @param current Reference to current control (for hot-reload)
     * @param temp Reference to temperature control (for hot-reload)
     * @param esc Reference to ESC controller (for hot-reload)
     */
    UartCli(ConfigManager& config, MoaBattControl& batt,
            MoaCurrentControl& current, MoaTempControl& temp,
            ESCController& esc);

    /**
     * @brief Initialize the CLI (prints welcome banner)
     */
    void begin();

    /**
     * @brief Poll for incoming serial data and process complete lines.
     *        Call this periodically from a task.
     */
    void poll();

private:
    ConfigManager& _config;
    MoaBattControl& _batt;
    MoaCurrentControl& _current;
    MoaTempControl& _temp;
    ESCController& _esc;

    char _lineBuf[UART_CLI_MAX_LINE];
    uint8_t _linePos;

    /**
     * @brief Process a complete command line
     * @param line Null-terminated command string
     */
    void processLine(const char* line);

    /**
     * @brief Handle 'set <key> <value>' command
     */
    void handleSet(const char* key, const char* value);

    /**
     * @brief Handle 'get <key>' command
     */
    void handleGet(const char* key);

    /**
     * @brief Print all settings
     */
    void handleDump();

    /**
     * @brief Print help text
     */
    void handleHelp();

    /**
     * @brief Apply current config to all devices (hot-reload)
     */
    void applyConfig();

    /**
     * @brief Print a single setting by key
     * @return true if key was found
     */
    bool printSetting(const char* key);

    /**
     * @brief Set a single setting by key and string value
     * @return true if key was found and value was valid
     */
    bool setSetting(const char* key, const char* value);
};
