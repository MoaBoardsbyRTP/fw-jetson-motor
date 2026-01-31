/**
 * @file MoaFlashLog.cpp
 * @brief Implementation of the MoaFlashLog class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaFlashLog.h"

MoaFlashLog::MoaFlashLog(const char* filename)
    : _filename(filename)
    , _flushIntervalMs(MOA_LOG_DEFAULT_FLUSH_INTERVAL_MS)
    , _lastFlushTime(0)
    , _initialized(false)
    , _entryCount(0)
    , _writeIndex(0)
    , _oldestIndex(0)
    , _ramBufferCount(0)
    , _dirty(false)
{
    memset(_entries, 0, sizeof(_entries));
    memset(_ramBuffer, 0, sizeof(_ramBuffer));
}

MoaFlashLog::~MoaFlashLog() {
    if (_initialized && _dirty) {
        flush();
    }
}

bool MoaFlashLog::begin() {
    if (!LittleFS.begin(true)) {  // true = format if mount fails
        return false;
    }
    
    _initialized = loadFromFlash();
    if (!_initialized) {
        // No existing log or read error - start fresh
        _entryCount = 0;
        _writeIndex = 0;
        _oldestIndex = 0;
        _initialized = true;
    }
    
    _lastFlushTime = millis();
    return _initialized;
}

void MoaFlashLog::update() {
    if (!_initialized) {
        return;
    }
    
    uint32_t now = millis();
    if ((now - _lastFlushTime) >= _flushIntervalMs) {
        flush();
        _lastFlushTime = now;
    }
}

void MoaFlashLog::setFlushInterval(uint32_t intervalMs) {
    _flushIntervalMs = intervalMs;
}

uint32_t MoaFlashLog::getFlushInterval() const {
    return _flushIntervalMs;
}

void MoaFlashLog::log(uint8_t type, uint8_t code, int16_t value, bool critical) {
    if (!_initialized) {
        return;
    }
    
    MoaLogEntry entry;
    entry.timestamp = millis();
    entry.type = type;
    entry.code = code;
    entry.value = value;
    
    // Add to RAM buffer
    if (_ramBufferCount < MOA_LOG_RAM_BUFFER_SIZE) {
        _ramBuffer[_ramBufferCount++] = entry;
    } else {
        // RAM buffer full - flush to circular buffer first
        flushRamBuffer();
        _ramBuffer[0] = entry;
        _ramBufferCount = 1;
    }
    
    _dirty = true;
    
    // Critical events trigger immediate flush to flash
    if (critical) {
        flush();
    }
}

void MoaFlashLog::logSystem(uint8_t code) {
    log(LOG_TYPE_SYSTEM, code, 0, false);
}

void MoaFlashLog::logButton(uint8_t code) {
    log(LOG_TYPE_BUTTON, code, 0, false);
}

void MoaFlashLog::logTemp(uint8_t code, int16_t tempX10) {
    // Overheat is critical
    bool critical = (code == LOG_TEMP_OVERHEAT);
    log(LOG_TYPE_TEMP, code, tempX10, critical);
}

void MoaFlashLog::logBatt(uint8_t code, int16_t voltageMv) {
    // Low battery is critical
    bool critical = (code == LOG_BATT_LOW);
    log(LOG_TYPE_BATT, code, voltageMv, critical);
}

void MoaFlashLog::logCurrent(uint8_t code, int16_t currentX10) {
    // Overcurrent and reverse overcurrent are critical
    bool critical = (code == LOG_CURRENT_OVERCURRENT || code == LOG_CURRENT_REVERSE);
    log(LOG_TYPE_CURRENT, code, currentX10, critical);
}

void MoaFlashLog::logState(uint8_t code) {
    log(LOG_TYPE_STATE, code, 0, false);
}

void MoaFlashLog::logError(uint8_t code, int16_t value) {
    // All errors are critical
    log(LOG_TYPE_ERROR, code, value, true);
}

void MoaFlashLog::flush() {
    if (!_initialized || !_dirty) {
        return;
    }
    
    // First flush RAM buffer to circular buffer
    flushRamBuffer();
    
    // Then save to flash
    if (saveToFlash()) {
        _dirty = false;
        _lastFlushTime = millis();
    }
}

void MoaFlashLog::clear() {
    _entryCount = 0;
    _writeIndex = 0;
    _oldestIndex = 0;
    _ramBufferCount = 0;
    _dirty = true;
    
    memset(_entries, 0, sizeof(_entries));
    memset(_ramBuffer, 0, sizeof(_ramBuffer));
    
    // Delete file
    if (_initialized) {
        LittleFS.remove(_filename);
        _dirty = false;
    }
}

size_t MoaFlashLog::getEntryCount() const {
    return _entryCount + _ramBufferCount;
}

bool MoaFlashLog::readEntry(size_t index, MoaLogEntry& entry) const {
    size_t totalCount = _entryCount + _ramBufferCount;
    
    if (index >= totalCount) {
        return false;
    }
    
    if (index < _entryCount) {
        // Entry is in circular buffer
        size_t actualIndex = (_oldestIndex + index) % MOA_LOG_MAX_ENTRIES;
        entry = _entries[actualIndex];
    } else {
        // Entry is in RAM buffer
        size_t ramIndex = index - _entryCount;
        entry = _ramBuffer[ramIndex];
    }
    
    return true;
}

String MoaFlashLog::toJson() const {
    String json = "{\"count\":";
    json += String(getEntryCount());
    json += ",\"entries\":[";
    
    size_t totalCount = getEntryCount();
    MoaLogEntry entry;
    
    for (size_t i = 0; i < totalCount; i++) {
        if (readEntry(i, entry)) {
            if (i > 0) {
                json += ",";
            }
            json += "{\"t\":";
            json += String(entry.timestamp);
            json += ",\"type\":";
            json += String(entry.type);
            json += ",\"code\":";
            json += String(entry.code);
            json += ",\"val\":";
            json += String(entry.value);
            json += "}";
        }
    }
    
    json += "]}";
    return json;
}

String MoaFlashLog::toJsonVerbose() const {
    String json = "{\"count\":";
    json += String(getEntryCount());
    json += ",\"entries\":[";
    
    size_t totalCount = getEntryCount();
    MoaLogEntry entry;
    
    for (size_t i = 0; i < totalCount; i++) {
        if (readEntry(i, entry)) {
            if (i > 0) {
                json += ",";
            }
            json += "{\"t\":";
            json += String(entry.timestamp);
            json += ",\"type\":\"";
            json += getTypeName(entry.type);
            json += "\",\"code\":\"";
            json += getCodeName(entry.type, entry.code);
            json += "\",\"val\":";
            json += String(entry.value);
            json += "}";
        }
    }
    
    json += "]}";
    return json;
}

void MoaFlashLog::dumpToSerial() const {
    Serial.println("=== MoaFlashLog Dump ===");
    Serial.print("Entries: ");
    Serial.println(getEntryCount());
    
    size_t totalCount = getEntryCount();
    MoaLogEntry entry;
    
    for (size_t i = 0; i < totalCount; i++) {
        if (readEntry(i, entry)) {
            Serial.print("[");
            Serial.print(i);
            Serial.print("] t=");
            Serial.print(entry.timestamp);
            Serial.print(" type=");
            Serial.print(getTypeName(entry.type));
            Serial.print(" code=");
            Serial.print(getCodeName(entry.type, entry.code));
            Serial.print(" val=");
            Serial.println(entry.value);
        }
    }
    
    Serial.println("========================");
}

bool MoaFlashLog::loadFromFlash() {
    if (!LittleFS.exists(_filename)) {
        return false;
    }
    
    File file = LittleFS.open(_filename, "r");
    if (!file) {
        return false;
    }
    
    // Read header: entryCount (2 bytes) + oldestIndex (2 bytes)
    uint16_t count = 0;
    uint16_t oldest = 0;
    
    if (file.read((uint8_t*)&count, 2) != 2 ||
        file.read((uint8_t*)&oldest, 2) != 2) {
        file.close();
        return false;
    }
    
    // Validate
    if (count > MOA_LOG_MAX_ENTRIES || oldest >= MOA_LOG_MAX_ENTRIES) {
        file.close();
        return false;
    }
    
    // Read entries
    size_t bytesRead = file.read((uint8_t*)_entries, sizeof(_entries));
    file.close();
    
    if (bytesRead != sizeof(_entries)) {
        return false;
    }
    
    _entryCount = count;
    _oldestIndex = oldest;
    _writeIndex = (_oldestIndex + _entryCount) % MOA_LOG_MAX_ENTRIES;
    
    return true;
}

bool MoaFlashLog::saveToFlash() {
    File file = LittleFS.open(_filename, "w");
    if (!file) {
        return false;
    }
    
    // Write header: entryCount (2 bytes) + oldestIndex (2 bytes)
    uint16_t count = static_cast<uint16_t>(_entryCount);
    uint16_t oldest = static_cast<uint16_t>(_oldestIndex);
    
    if (file.write((uint8_t*)&count, 2) != 2 ||
        file.write((uint8_t*)&oldest, 2) != 2) {
        file.close();
        return false;
    }
    
    // Write entries
    size_t bytesWritten = file.write((uint8_t*)_entries, sizeof(_entries));
    file.close();
    
    return bytesWritten == sizeof(_entries);
}

void MoaFlashLog::addEntry(const MoaLogEntry& entry) {
    _entries[_writeIndex] = entry;
    _writeIndex = (_writeIndex + 1) % MOA_LOG_MAX_ENTRIES;
    
    if (_entryCount < MOA_LOG_MAX_ENTRIES) {
        _entryCount++;
    } else {
        // Buffer is full - oldest entry is overwritten
        _oldestIndex = (_oldestIndex + 1) % MOA_LOG_MAX_ENTRIES;
    }
}

void MoaFlashLog::flushRamBuffer() {
    for (size_t i = 0; i < _ramBufferCount; i++) {
        addEntry(_ramBuffer[i]);
    }
    _ramBufferCount = 0;
}

const char* MoaFlashLog::getTypeName(uint8_t type) const {
    switch (type) {
        case LOG_TYPE_SYSTEM:  return "SYSTEM";
        case LOG_TYPE_BUTTON:  return "BUTTON";
        case LOG_TYPE_TEMP:    return "TEMP";
        case LOG_TYPE_BATT:    return "BATT";
        case LOG_TYPE_CURRENT: return "CURRENT";
        case LOG_TYPE_STATE:   return "STATE";
        case LOG_TYPE_ERROR:   return "ERROR";
        default:               return "UNKNOWN";
    }
}

const char* MoaFlashLog::getCodeName(uint8_t type, uint8_t code) const {
    switch (type) {
        case LOG_TYPE_SYSTEM:
            switch (code) {
                case LOG_SYS_BOOT:           return "BOOT";
                case LOG_SYS_SHUTDOWN:       return "SHUTDOWN";
                case LOG_SYS_CONFIG_ENTER:   return "CONFIG_ENTER";
                case LOG_SYS_CONFIG_EXIT:    return "CONFIG_EXIT";
                case LOG_SYS_WATCHDOG_RESET: return "WATCHDOG";
                default:                     return "?";
            }
        case LOG_TYPE_BUTTON:
            switch (code) {
                case LOG_BTN_STOP_PRESS:     return "STOP";
                case LOG_BTN_STOP_LONG:      return "STOP_LONG";
                case LOG_BTN_25_PRESS:       return "25%";
                case LOG_BTN_50_PRESS:       return "50%";
                case LOG_BTN_75_PRESS:       return "75%";
                case LOG_BTN_100_PRESS:      return "100%";
                default:                     return "?";
            }
        case LOG_TYPE_TEMP:
            switch (code) {
                case LOG_TEMP_CROSSED_ABOVE: return "ABOVE";
                case LOG_TEMP_CROSSED_BELOW: return "BELOW";
                case LOG_TEMP_OVERHEAT:      return "OVERHEAT";
                default:                     return "?";
            }
        case LOG_TYPE_BATT:
            switch (code) {
                case LOG_BATT_HIGH:          return "HIGH";
                case LOG_BATT_MEDIUM:        return "MEDIUM";
                case LOG_BATT_LOW:           return "LOW";
                default:                     return "?";
            }
        case LOG_TYPE_CURRENT:
            switch (code) {
                case LOG_CURRENT_NORMAL:     return "NORMAL";
                case LOG_CURRENT_OVERCURRENT:return "OVERCURRENT";
                case LOG_CURRENT_REVERSE:    return "REVERSE";
                default:                     return "?";
            }
        case LOG_TYPE_STATE:
            switch (code) {
                case LOG_STATE_TO_INIT:      return "INIT";
                case LOG_STATE_TO_IDLE:      return "IDLE";
                case LOG_STATE_TO_SURFING:   return "SURFING";
                case LOG_STATE_TO_OVERHEAT:  return "OVERHEAT";
                case LOG_STATE_TO_OVERCURRENT:return "OVERCURRENT";
                case LOG_STATE_TO_BATT_LOW:  return "BATT_LOW";
                default:                     return "?";
            }
        case LOG_TYPE_ERROR:
            switch (code) {
                case LOG_ERR_I2C_FAIL:       return "I2C_FAIL";
                case LOG_ERR_SENSOR_FAIL:    return "SENSOR_FAIL";
                case LOG_ERR_FLASH_FAIL:     return "FLASH_FAIL";
                case LOG_ERR_QUEUE_FULL:     return "QUEUE_FULL";
                default:                     return "?";
            }
        default:
            return "?";
    }
}
