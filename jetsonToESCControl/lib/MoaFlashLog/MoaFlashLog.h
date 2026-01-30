/**
 * @file MoaFlashLog.h
 * @brief Compact flash-based event logging with circular buffer
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * This library provides persistent event logging to internal flash using LittleFS.
 * Features:
 * - Circular buffer of 128 entries (oldest overwritten)
 * - Compact 8-byte binary format per entry
 * - RAM buffering with periodic flush (default: 1 minute)
 * - Immediate flush on critical events (overcurrent, overtemperature)
 * - JSON export for webserver retrieval
 * - Relative timestamps (millis())
 * 
 * ## Log Entry Format (8 bytes)
 * | Field     | Size    | Description                          |
 * |-----------|---------|--------------------------------------|
 * | timestamp | 4 bytes | millis() at event time               |
 * | type      | 1 byte  | Event category (LOG_TYPE_*)          |
 * | code      | 1 byte  | Specific event code                  |
 * | value     | 2 bytes | Associated value (temp, voltage, etc)|
 */

#pragma once

#include <Arduino.h>
#include <LittleFS.h>

/**
 * @brief Maximum number of log entries (circular buffer size)
 */
#define MOA_LOG_MAX_ENTRIES 128

/**
 * @brief Size of each log entry in bytes
 */
#define MOA_LOG_ENTRY_SIZE 8

/**
 * @brief Default flush interval in milliseconds (1 minute)
 */
#define MOA_LOG_DEFAULT_FLUSH_INTERVAL_MS 60000

/**
 * @brief RAM buffer size (entries to accumulate before flush)
 */
#define MOA_LOG_RAM_BUFFER_SIZE 8

/**
 * @brief Default log filename
 */
#define MOA_LOG_DEFAULT_FILENAME "/moa_log.bin"

/**
 * @brief Log event types (categories)
 */
enum MoaLogType : uint8_t {
    LOG_TYPE_SYSTEM     = 0x00,   ///< System events (boot, shutdown, config)
    LOG_TYPE_BUTTON     = 0x10,   ///< Button events
    LOG_TYPE_TEMP       = 0x20,   ///< Temperature events
    LOG_TYPE_BATT       = 0x30,   ///< Battery events
    LOG_TYPE_CURRENT    = 0x40,   ///< Current events
    LOG_TYPE_STATE      = 0x50,   ///< State machine transitions
    LOG_TYPE_ERROR      = 0xF0    ///< Errors and failures
};

/**
 * @brief System event codes (LOG_TYPE_SYSTEM)
 */
enum MoaLogSystemCode : uint8_t {
    LOG_SYS_BOOT            = 0x01,   ///< System boot
    LOG_SYS_SHUTDOWN        = 0x02,   ///< Graceful shutdown
    LOG_SYS_CONFIG_ENTER    = 0x03,   ///< Entered config mode
    LOG_SYS_CONFIG_EXIT     = 0x04,   ///< Exited config mode
    LOG_SYS_WATCHDOG_RESET  = 0x05    ///< Watchdog triggered reset
};

/**
 * @brief Button event codes (LOG_TYPE_BUTTON)
 */
enum MoaLogButtonCode : uint8_t {
    LOG_BTN_STOP_PRESS      = 0x01,
    LOG_BTN_STOP_LONG       = 0x02,
    LOG_BTN_25_PRESS        = 0x03,
    LOG_BTN_50_PRESS        = 0x04,
    LOG_BTN_75_PRESS        = 0x05,
    LOG_BTN_100_PRESS       = 0x06
};

/**
 * @brief Temperature event codes (LOG_TYPE_TEMP)
 */
enum MoaLogTempCode : uint8_t {
    LOG_TEMP_CROSSED_ABOVE  = 0x01,   ///< Temperature crossed above threshold
    LOG_TEMP_CROSSED_BELOW  = 0x02,   ///< Temperature crossed below threshold
    LOG_TEMP_OVERHEAT       = 0x03    ///< Critical overheat (immediate flush)
};

/**
 * @brief Battery event codes (LOG_TYPE_BATT)
 */
enum MoaLogBattCode : uint8_t {
    LOG_BATT_HIGH           = 0x01,
    LOG_BATT_MEDIUM         = 0x02,
    LOG_BATT_LOW            = 0x03    ///< Critical low battery (immediate flush)
};

/**
 * @brief Current event codes (LOG_TYPE_CURRENT)
 */
enum MoaLogCurrentCode : uint8_t {
    LOG_CURRENT_NORMAL      = 0x01,
    LOG_CURRENT_OVERCURRENT = 0x02,   ///< Critical overcurrent (immediate flush)
    LOG_CURRENT_REVERSE     = 0x03    ///< Critical reverse overcurrent (immediate flush)
};

/**
 * @brief State machine event codes (LOG_TYPE_STATE)
 */
enum MoaLogStateCode : uint8_t {
    LOG_STATE_TO_INIT       = 0x01,
    LOG_STATE_TO_IDLE       = 0x02,
    LOG_STATE_TO_SURFING    = 0x03,
    LOG_STATE_TO_OVERHEAT   = 0x04,
    LOG_STATE_TO_OVERCURRENT= 0x05,
    LOG_STATE_TO_BATT_LOW   = 0x06
};

/**
 * @brief Error event codes (LOG_TYPE_ERROR)
 */
enum MoaLogErrorCode : uint8_t {
    LOG_ERR_I2C_FAIL        = 0x01,   ///< I2C communication failure
    LOG_ERR_SENSOR_FAIL     = 0x02,   ///< Sensor read failure
    LOG_ERR_FLASH_FAIL      = 0x03,   ///< Flash write failure
    LOG_ERR_QUEUE_FULL      = 0x04    ///< Event queue overflow
};

/**
 * @brief Log entry structure (8 bytes)
 */
struct MoaLogEntry {
    uint32_t timestamp;    ///< millis() at event time
    uint8_t type;          ///< Event type (MoaLogType)
    uint8_t code;          ///< Event code within type
    int16_t value;         ///< Associated value
} __attribute__((packed));

/**
 * @brief Flash-based event logger with circular buffer
 * 
 * MoaFlashLog provides persistent event logging to internal flash with:
 * - Circular buffer of 128 entries
 * - RAM buffering to reduce flash wear
 * - Periodic flush (default: 1 minute)
 * - Immediate flush on critical events
 * - JSON export for webserver
 * 
 * ## Usage Example
 * @code
 * MoaFlashLog logger;
 * logger.begin();
 * 
 * // Log events
 * logger.logSystem(LOG_SYS_BOOT);
 * logger.logTemp(LOG_TEMP_CROSSED_ABOVE, 425);  // 42.5°C
 * logger.logCurrent(LOG_CURRENT_OVERCURRENT, 1550);  // 155.0A (critical - auto flush)
 * 
 * // Periodic call (e.g., in main loop or task)
 * logger.update();  // Handles timed flush
 * 
 * // For webserver
 * String json = logger.toJson();
 * @endcode
 */
class MoaFlashLog {
public:
    /**
     * @brief Construct a new MoaFlashLog object
     * 
     * @param filename Log file path (default: "/moa_log.bin")
     */
    MoaFlashLog(const char* filename = MOA_LOG_DEFAULT_FILENAME);

    /**
     * @brief Destructor - flushes pending entries
     */
    ~MoaFlashLog();

    /**
     * @brief Initialize the logger
     * 
     * Mounts LittleFS and loads existing log file or creates new one.
     * 
     * @return true if initialization successful
     * @return false if filesystem mount or file operation failed
     */
    bool begin();

    /**
     * @brief Update logger - call periodically
     * 
     * Handles timed flush based on flush interval.
     * Call this from main loop or a periodic task.
     */
    void update();

    /**
     * @brief Set the flush interval
     * 
     * @param intervalMs Flush interval in milliseconds (default: 60000)
     */
    void setFlushInterval(uint32_t intervalMs);

    /**
     * @brief Get the current flush interval
     * @return uint32_t Flush interval in milliseconds
     */
    uint32_t getFlushInterval() const;

    // === Generic Logging ===

    /**
     * @brief Log a generic event
     * 
     * @param type Event type (MoaLogType)
     * @param code Event code
     * @param value Associated value (default: 0)
     * @param critical If true, triggers immediate flush
     */
    void log(uint8_t type, uint8_t code, int16_t value = 0, bool critical = false);

    // === Convenience Logging Methods ===

    /**
     * @brief Log a system event
     * @param code System event code (MoaLogSystemCode)
     */
    void logSystem(uint8_t code);

    /**
     * @brief Log a button event
     * @param code Button event code (MoaLogButtonCode)
     */
    void logButton(uint8_t code);

    /**
     * @brief Log a temperature event
     * @param code Temperature event code (MoaLogTempCode)
     * @param tempX10 Temperature × 10 (e.g., 425 = 42.5°C)
     */
    void logTemp(uint8_t code, int16_t tempX10);

    /**
     * @brief Log a battery event
     * @param code Battery event code (MoaLogBattCode)
     * @param voltageMv Voltage in millivolts
     */
    void logBatt(uint8_t code, int16_t voltageMv);

    /**
     * @brief Log a current event
     * @param code Current event code (MoaLogCurrentCode)
     * @param currentX10 Current × 10 (e.g., 1550 = 155.0A)
     */
    void logCurrent(uint8_t code, int16_t currentX10);

    /**
     * @brief Log a state machine transition
     * @param code State code (MoaLogStateCode)
     */
    void logState(uint8_t code);

    /**
     * @brief Log an error
     * @param code Error code (MoaLogErrorCode)
     * @param value Additional error info
     */
    void logError(uint8_t code, int16_t value = 0);

    // === Log Management ===

    /**
     * @brief Force flush RAM buffer to flash
     */
    void flush();

    /**
     * @brief Clear all log entries
     */
    void clear();

    /**
     * @brief Get the number of log entries
     * @return size_t Number of entries (0 to MOA_LOG_MAX_ENTRIES)
     */
    size_t getEntryCount() const;

    /**
     * @brief Read a specific log entry
     * 
     * @param index Entry index (0 = oldest)
     * @param entry Output entry structure
     * @return true if entry read successfully
     * @return false if index out of range
     */
    bool readEntry(size_t index, MoaLogEntry& entry) const;

    // === Export ===

    /**
     * @brief Export log as JSON string
     * 
     * Format:
     * @code
     * {
     *   "count": 42,
     *   "entries": [
     *     {"t": 12345, "type": 32, "code": 1, "val": 425},
     *     ...
     *   ]
     * }
     * @endcode
     * 
     * @return String JSON representation of log
     */
    String toJson() const;

    /**
     * @brief Export log as human-readable JSON
     * 
     * Includes type/code names for easier reading.
     * 
     * @return String Human-readable JSON
     */
    String toJsonVerbose() const;

    /**
     * @brief Dump log to Serial (for debugging)
     */
    void dumpToSerial() const;

private:
    const char* _filename;                         ///< Log file path
    uint32_t _flushIntervalMs;                     ///< Flush interval
    uint32_t _lastFlushTime;                       ///< Last flush timestamp
    bool _initialized;                             ///< Initialization flag
    
    MoaLogEntry _entries[MOA_LOG_MAX_ENTRIES];     ///< Circular buffer in RAM
    size_t _entryCount;                            ///< Number of valid entries
    size_t _writeIndex;                            ///< Next write position
    size_t _oldestIndex;                           ///< Oldest entry position
    
    MoaLogEntry _ramBuffer[MOA_LOG_RAM_BUFFER_SIZE]; ///< Pending entries
    size_t _ramBufferCount;                        ///< Entries in RAM buffer
    bool _dirty;                                   ///< Unsaved changes flag

    /**
     * @brief Load log from flash file
     * @return true if loaded successfully
     */
    bool loadFromFlash();

    /**
     * @brief Save log to flash file
     * @return true if saved successfully
     */
    bool saveToFlash();

    /**
     * @brief Add entry to circular buffer
     * @param entry Entry to add
     */
    void addEntry(const MoaLogEntry& entry);

    /**
     * @brief Flush RAM buffer to circular buffer
     */
    void flushRamBuffer();

    /**
     * @brief Get type name for verbose JSON
     * @param type Event type
     * @return const char* Type name string
     */
    const char* getTypeName(uint8_t type) const;

    /**
     * @brief Get code name for verbose JSON
     * @param type Event type
     * @param code Event code
     * @return const char* Code name string
     */
    const char* getCodeName(uint8_t type, uint8_t code) const;
};
