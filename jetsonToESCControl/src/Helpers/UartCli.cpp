/**
 * @file UartCli.cpp
 * @brief Implementation of the UartCli class
 * @author Oscar Martinez
 * @date 2026-02-15
 */

#include "UartCli.h"
#include "MoaBattControl.h"
#include "MoaCurrentControl.h"
#include "MoaTempControl.h"
#include "ESCController.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "CLI";

UartCli::UartCli(ConfigManager& config, MoaBattControl& batt,
                 MoaCurrentControl& current, MoaTempControl& temp,
                 ESCController& esc)
    : _config(config)
    , _batt(batt)
    , _current(current)
    , _temp(temp)
    , _esc(esc)
    , _linePos(0)
{
    memset(_lineBuf, 0, sizeof(_lineBuf));
}

void UartCli::begin() {
    Serial.println();
    Serial.println(F("=== Moa UART CLI ==="));
    Serial.println(F("Type 'help' for commands."));
    Serial.print(F("> "));
}

void UartCli::poll() {
    while (Serial.available()) {
        char c = Serial.read();

        // Handle backspace
        if (c == '\b' || c == 127) {
            if (_linePos > 0) {
                _linePos--;
                Serial.print(F("\b \b"));
            }
            continue;
        }

        // Handle line completion
        if (c == '\n' || c == '\r') {
            if (_linePos > 0) {
                Serial.println();
                _lineBuf[_linePos] = '\0';
                processLine(_lineBuf);
                _linePos = 0;
                memset(_lineBuf, 0, sizeof(_lineBuf));
            }
            Serial.print(F("> "));
            continue;
        }

        // Buffer the character
        if (_linePos < UART_CLI_MAX_LINE - 1) {
            _lineBuf[_linePos++] = c;
            Serial.print(c);  // echo
        }
    }
}

void UartCli::processLine(const char* line) {
    // Skip leading whitespace
    while (*line == ' ') line++;
    if (*line == '\0') return;

    // Parse command
    char cmd[16] = {0};
    char arg1[32] = {0};
    char arg2[32] = {0};

    int parsed = sscanf(line, "%15s %31s %31s", cmd, arg1, arg2);

    if (strcasecmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        handleHelp();
    } else if (strcasecmp(cmd, "get") == 0 && parsed >= 2) {
        handleGet(arg1);
    } else if (strcasecmp(cmd, "set") == 0 && parsed >= 3) {
        handleSet(arg1, arg2);
    } else if (strcasecmp(cmd, "dump") == 0) {
        handleDump();
    } else if (strcasecmp(cmd, "save") == 0) {
        if (_config.save()) {
            Serial.println(F("OK: Settings saved to NVS"));
        } else {
            Serial.println(F("ERR: Save failed"));
        }
    } else if (strcasecmp(cmd, "apply") == 0) {
        applyConfig();
        Serial.println(F("OK: Settings applied to devices"));
    } else if (strcasecmp(cmd, "reset") == 0) {
        _config.resetToDefaults();
        applyConfig();
        Serial.println(F("OK: Reset to defaults, saved, and applied"));
    } else {
        Serial.print(F("ERR: Unknown command '"));
        Serial.print(cmd);
        Serial.println(F("'. Type 'help'."));
    }
}

void UartCli::handleSet(const char* key, const char* value) {
    if (setSetting(key, value)) {
        Serial.print(F("OK: "));
        Serial.print(key);
        Serial.print(F(" = "));
        printSetting(key);
    } else {
        Serial.print(F("ERR: Unknown key '"));
        Serial.print(key);
        Serial.println(F("'"));
    }
}

void UartCli::handleGet(const char* key) {
    if (strcmp(key, "all") == 0) {
        handleDump();
        return;
    }
    if (!printSetting(key)) {
        Serial.print(F("ERR: Unknown key '"));
        Serial.print(key);
        Serial.println(F("'"));
    }
}

void UartCli::handleDump() {
    Serial.println(F("--- Surfing Timers (ms) ---"));
    printSetting("esc_t25");
    printSetting("esc_t50");
    printSetting("esc_t75");
    printSetting("esc_t100");
    printSetting("esc_t75_100");

    Serial.println(F("--- Throttle Percentages ---"));
    printSetting("esc_eco");
    printSetting("esc_paddle");
    printSetting("esc_break");
    printSetting("esc_full");
    printSetting("esc_ramp");

    Serial.println(F("--- Battery Thresholds (V) ---"));
    printSetting("batt_high");
    printSetting("batt_med");
    printSetting("batt_low");
    printSetting("batt_hyst");

    Serial.println(F("--- Temperature Thresholds (C) ---"));
    printSetting("temp_tgt");
    printSetting("temp_hyst");

    Serial.println(F("--- Current Thresholds (A) ---"));
    printSetting("curr_oc");
    printSetting("curr_rev");
    printSetting("curr_hyst");
}

void UartCli::handleHelp() {
    Serial.println(F("Commands:"));
    Serial.println(F("  get <key>       Read a setting"));
    Serial.println(F("  get all         Read all settings"));
    Serial.println(F("  set <key> <val> Write a setting (in-memory only)"));
    Serial.println(F("  dump            Print all settings"));
    Serial.println(F("  save            Persist to NVS"));
    Serial.println(F("  apply           Hot-reload to devices"));
    Serial.println(F("  reset           Restore defaults, save, apply"));
    Serial.println(F("  help            Show this help"));
    Serial.println();
    Serial.println(F("Keys:"));
    Serial.println(F("  esc_t25, esc_t50, esc_t75, esc_t100, esc_t75_100  (ms)"));
    Serial.println(F("  esc_eco, esc_paddle, esc_break, esc_full           (%)"));
    Serial.println(F("  esc_ramp                                           (%/s)"));
    Serial.println(F("  batt_high, batt_med, batt_low, batt_hyst           (V)"));
    Serial.println(F("  temp_tgt, temp_hyst                                (C)"));
    Serial.println(F("  curr_oc, curr_rev, curr_hyst                       (A)"));
    Serial.println();
    Serial.println(F("Workflow: set <key> <val> → apply → (test) → save"));
}

void UartCli::applyConfig() {
    _config.applyTo(_batt, _current, _temp, _esc);
}

bool UartCli::printSetting(const char* key) {
    // Surfing timers
    if (strcmp(key, "esc_t25") == 0)      { Serial.printf("  %-12s = %lu ms\n", key, _config.escTime25); return true; }
    if (strcmp(key, "esc_t50") == 0)      { Serial.printf("  %-12s = %lu ms\n", key, _config.escTime50); return true; }
    if (strcmp(key, "esc_t75") == 0)      { Serial.printf("  %-12s = %lu ms\n", key, _config.escTime75); return true; }
    if (strcmp(key, "esc_t100") == 0)     { Serial.printf("  %-12s = %lu ms\n", key, _config.escTime100); return true; }
    if (strcmp(key, "esc_t75_100") == 0)  { Serial.printf("  %-12s = %lu ms\n", key, _config.escTime75From100); return true; }

    // Throttle percentages
    if (strcmp(key, "esc_eco") == 0)      { Serial.printf("  %-12s = %u %%\n", key, _config.escEcoMode); return true; }
    if (strcmp(key, "esc_paddle") == 0)   { Serial.printf("  %-12s = %u %%\n", key, _config.escPaddleMode); return true; }
    if (strcmp(key, "esc_break") == 0)    { Serial.printf("  %-12s = %u %%\n", key, _config.escBreakingMode); return true; }
    if (strcmp(key, "esc_full") == 0)     { Serial.printf("  %-12s = %u %%\n", key, _config.escFullThrottle); return true; }
    if (strcmp(key, "esc_ramp") == 0)     { Serial.printf("  %-12s = %.1f %%/s\n", key, _config.escRampRate); return true; }

    // Battery
    if (strcmp(key, "batt_high") == 0)    { Serial.printf("  %-12s = %.2f V\n", key, _config.battHigh); return true; }
    if (strcmp(key, "batt_med") == 0)     { Serial.printf("  %-12s = %.2f V\n", key, _config.battMedium); return true; }
    if (strcmp(key, "batt_low") == 0)     { Serial.printf("  %-12s = %.2f V\n", key, _config.battLow); return true; }
    if (strcmp(key, "batt_hyst") == 0)    { Serial.printf("  %-12s = %.2f V\n", key, _config.battHysteresis); return true; }

    // Temperature
    if (strcmp(key, "temp_tgt") == 0)     { Serial.printf("  %-12s = %.1f C\n", key, _config.tempTarget); return true; }
    if (strcmp(key, "temp_hyst") == 0)    { Serial.printf("  %-12s = %.1f C\n", key, _config.tempHysteresis); return true; }

    // Current
    if (strcmp(key, "curr_oc") == 0)      { Serial.printf("  %-12s = %.1f A\n", key, _config.currentOvercurrent); return true; }
    if (strcmp(key, "curr_rev") == 0)     { Serial.printf("  %-12s = %.1f A\n", key, _config.currentReverse); return true; }
    if (strcmp(key, "curr_hyst") == 0)    { Serial.printf("  %-12s = %.1f A\n", key, _config.currentHysteresis); return true; }

    return false;
}

bool UartCli::setSetting(const char* key, const char* value) {
    // Surfing timers (uint32_t)
    if (strcmp(key, "esc_t25") == 0)      { _config.escTime25 = strtoul(value, nullptr, 10); return true; }
    if (strcmp(key, "esc_t50") == 0)      { _config.escTime50 = strtoul(value, nullptr, 10); return true; }
    if (strcmp(key, "esc_t75") == 0)      { _config.escTime75 = strtoul(value, nullptr, 10); return true; }
    if (strcmp(key, "esc_t100") == 0)     { _config.escTime100 = strtoul(value, nullptr, 10); return true; }
    if (strcmp(key, "esc_t75_100") == 0)  { _config.escTime75From100 = strtoul(value, nullptr, 10); return true; }

    // Throttle percentages (uint8_t, clamped 0-100)
    if (strcmp(key, "esc_eco") == 0)      { uint8_t v = (uint8_t)atoi(value); if (v > 100) v = 100; _config.escEcoMode = v; return true; }
    if (strcmp(key, "esc_paddle") == 0)   { uint8_t v = (uint8_t)atoi(value); if (v > 100) v = 100; _config.escPaddleMode = v; return true; }
    if (strcmp(key, "esc_break") == 0)    { uint8_t v = (uint8_t)atoi(value); if (v > 100) v = 100; _config.escBreakingMode = v; return true; }
    if (strcmp(key, "esc_full") == 0)     { uint8_t v = (uint8_t)atoi(value); if (v > 100) v = 100; _config.escFullThrottle = v; return true; }
    if (strcmp(key, "esc_ramp") == 0)     { _config.escRampRate = atof(value); return true; }

    // Battery (float)
    if (strcmp(key, "batt_high") == 0)    { _config.battHigh = atof(value); return true; }
    if (strcmp(key, "batt_med") == 0)     { _config.battMedium = atof(value); return true; }
    if (strcmp(key, "batt_low") == 0)     { _config.battLow = atof(value); return true; }
    if (strcmp(key, "batt_hyst") == 0)    { _config.battHysteresis = atof(value); return true; }

    // Temperature (float)
    if (strcmp(key, "temp_tgt") == 0)     { _config.tempTarget = atof(value); return true; }
    if (strcmp(key, "temp_hyst") == 0)    { _config.tempHysteresis = atof(value); return true; }

    // Current (float)
    if (strcmp(key, "curr_oc") == 0)      { _config.currentOvercurrent = atof(value); return true; }
    if (strcmp(key, "curr_rev") == 0)     { _config.currentReverse = atof(value); return true; }
    if (strcmp(key, "curr_hyst") == 0)    { _config.currentHysteresis = atof(value); return true; }

    return false;
}
