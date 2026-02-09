/**
 * @file MoaMcpDevice.cpp
 * @brief Implementation of the MoaMcpDevice class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaMcpDevice.h"
#include "esp_log.h"

static const char* TAG = "MCP";

MoaMcpDevice::MoaMcpDevice(uint8_t i2cAddr)
    : _i2cAddr(i2cAddr)
    , _mutexTimeoutMs(MOA_MCP_MUTEX_TIMEOUT_MS)
    , _initialized(false)
    , _resetPin(PIN_I2C_RESET)
{
    _mutex = xSemaphoreCreateMutex();
}

void MoaMcpDevice::hardwareReset() {
    ESP_LOGD(TAG, "Hardware reset (pin=%d)", _resetPin);
    // Configure reset pin as output if not already done
    pinMode(_resetPin, OUTPUT);
    
    // Active LOW reset pulse - minimum 1μs required, use 2μs for safety
    digitalWrite(_resetPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_resetPin, HIGH);
    
    // Wait for MCP23018 to stabilize (1ms as per datasheet)
    delay(1);
}

bool MoaMcpDevice::recover(TwoWire* wire) {
    ESP_LOGW(TAG, "Attempting MCP23018 recovery...");
    // Attempt hardware reset
    hardwareReset();
    
    // Re-initialize I2C communication
    if (!acquireMutex()) {
        return false;
    }
    
    _initialized = _mcp.begin_I2C(_i2cAddr, wire);
    ESP_LOGI(TAG, "Recovery %s", _initialized ? "succeeded" : "FAILED");
    
    releaseMutex();
    return _initialized;
}

uint8_t MoaMcpDevice::readInterruptCapturePortA() {
    if (!acquireMutex()) {
        return 0;
    }
    
    // Read INTCAPA via our custom Adafruit_MCP23X18 method
    uint8_t value = _mcp.readIntCapA();
    
    releaseMutex();
    return value;
}

bool MoaMcpDevice::isInterruptActive(uint8_t intPin) {
    // Check if the interrupt pin is still asserted (LOW)
    return digitalRead(intPin) == LOW;
}

MoaMcpDevice::~MoaMcpDevice() {
    if (_mutex != nullptr) {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
}

bool MoaMcpDevice::begin(TwoWire* wire) {
    // Perform hardware reset first to ensure known state
    hardwareReset();
    
    if (!acquireMutex()) {
        return false;
    }
    
    _initialized = _mcp.begin_I2C(_i2cAddr, wire);
    ESP_LOGI(TAG, "MCP23018 begin: %s (addr=0x%02X)", _initialized ? "OK" : "FAILED", _i2cAddr);
    
    releaseMutex();
    return _initialized;
}

bool MoaMcpDevice::isInitialized() const {
    return _initialized;
}

SemaphoreHandle_t MoaMcpDevice::getMutex() {
    return _mutex;
}

Adafruit_MCP23X18& MoaMcpDevice::getMcp() {
    return _mcp;
}

void MoaMcpDevice::setMutexTimeout(uint32_t timeoutMs) {
    _mutexTimeoutMs = timeoutMs;
}

uint32_t MoaMcpDevice::getMutexTimeout() const {
    return _mutexTimeoutMs;
}

uint8_t MoaMcpDevice::readPortA() {
    if (!acquireMutex()) {
        return 0;
    }
    
    uint8_t value = _mcp.readGPIOA();
    
    releaseMutex();
    return value;
}

void MoaMcpDevice::configurePortA(uint8_t mask, uint8_t mode) {
    if (!acquireMutex()) {
        return;
    }
    
    for (uint8_t i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            _mcp.pinMode(i, mode);  // Port A pins are 0-7
            _mcp.setPullup(i, mode == PULLUP);
        }
    }
    
    releaseMutex();
}

void MoaMcpDevice::configurePortA(uint8_t mask, uint8_t mode, uint8_t pullupMask) {
    if (!acquireMutex()) {
        return;
    }
    
    // Build direction byte: 1=input, 0=output for masked pins
    uint8_t dir = (mode == OUTPUT) ? ~mask : mask;
    _mcp.configGPIOA(dir, pullupMask);
    
    releaseMutex();
}

void MoaMcpDevice::enableInterruptPortA(uint8_t mask, uint8_t defaultValue) {
    if (!acquireMutex()) {
        return;
    }
    
    for (uint8_t i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            _mcp.setupInterruptPin(i, CHANGE);
        }
    }
    
    releaseMutex();
}

uint8_t MoaMcpDevice::readPortB() {
    if (!acquireMutex()) {
        return 0;
    }
    
    uint8_t value = _mcp.readGPIOB();
    
    releaseMutex();
    return value;
}

void MoaMcpDevice::writePortB(uint8_t value) {
    if (!acquireMutex()) {
        return;
    }
    
    _mcp.writeGPIOB(value);
    
    releaseMutex();
}

void MoaMcpDevice::configurePortB(uint8_t mask, uint8_t mode) {
    if (!acquireMutex()) {
        return;
    }
    
    for (uint8_t i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            _mcp.pinMode(8 + i, mode);  // Port B pins are 8-15
            _mcp.setPullup(8 + i, mode == PULLUP);
        }
    }
    
    releaseMutex();
}

void MoaMcpDevice::configurePortB(uint8_t mask, uint8_t mode, uint8_t pullupMask) {
    if (!acquireMutex()) {
        return;
    }
    
    // Build direction byte: 1=input, 0=output for masked pins
    uint8_t dir = (mode == OUTPUT) ? ~mask : mask;
    _mcp.configGPIOB(dir, pullupMask);
    
    releaseMutex();
}

void MoaMcpDevice::setPinMode(uint8_t pin, uint8_t mode) {
    if (!acquireMutex()) {
        return;
    }
    
    _mcp.pinMode(pin, mode);
    _mcp.setPullup(pin, mode == PULLUP);
    
    releaseMutex();
}

void MoaMcpDevice::writePin(uint8_t pin, bool value) {
    if (!acquireMutex()) {
        return;
    }
    
    _mcp.digitalWrite(pin, value ? HIGH : LOW);
    
    releaseMutex();
}

bool MoaMcpDevice::readPin(uint8_t pin) {
    if (!acquireMutex()) {
        return false;
    }
    
    bool value = _mcp.digitalRead(pin) == HIGH;
    
    releaseMutex();
    return value;
}

bool MoaMcpDevice::acquireMutex() {
    if (_mutex == nullptr) {
        ESP_LOGE(TAG, "Mutex is null!");
        return false;
    }
    bool acquired = xSemaphoreTake(_mutex, pdMS_TO_TICKS(_mutexTimeoutMs)) == pdTRUE;
    if (!acquired) {
        ESP_LOGW(TAG, "Mutex acquire timeout (%dms)", _mutexTimeoutMs);
    }
    return acquired;
}

void MoaMcpDevice::releaseMutex() {
    if (_mutex != nullptr) {
        xSemaphoreGive(_mutex);
    }
}
