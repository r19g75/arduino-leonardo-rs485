#include "ModbusAnalyzer.h"

const unsigned long ModbusAnalyzer::BAUD_RATES[] = {9600, 19200, 38400, 57600, 115200};
const uint8_t ModbusAnalyzer::BAUD_COUNT = sizeof(BAUD_RATES) / sizeof(BAUD_RATES[0]);

ModbusAnalyzer::ModbusAnalyzer(HardwareSerial& modbusSerial)
    : _serial(modbusSerial)
    , _analyzing(false)
    , _baudIndex(0)
    , _bufferIndex(0)
    , _lastActivityTime(0)
    , _lastFrameTime(0)
{
    memset(&_masterInfo, 0, sizeof(MasterInfo));
    _masterInfo.minQueryInterval = UINT32_MAX;
    _masterInfo.maxQueryInterval = 0;
}

void ModbusAnalyzer::begin() {
    _serial.begin(BAUD_RATES[0]);
}

void ModbusAnalyzer::startAnalysis() {
    _analyzing = true;
    _baudIndex = 0;
    _bufferIndex = 0;
    _lastActivityTime = millis();
    _lastFrameTime = millis();
    memset(&_masterInfo, 0, sizeof(MasterInfo));
    _masterInfo.minQueryInterval = UINT32_MAX;
    _masterInfo.maxQueryInterval = 0;
    _serial.begin(BAUD_RATES[0]);
    Serial.println(F("\nRozpoczynam nasluchiwanie magistrali..."));
}

void ModbusAnalyzer::stop() {
    _analyzing = false;
    Serial.println(F("Zatrzymano nasluchiwanie"));
}

void ModbusAnalyzer::update() {
    if (!_analyzing) return;

    if (_serial.available()) {
        checkCollision();
        
        while (_serial.available() && _bufferIndex < MAX_BUFFER) {
            _buffer[_bufferIndex++] = _serial.read();
            _lastActivityTime = millis();
        }
        
        _masterInfo.totalFrames++;
        
        if (_bufferIndex >= 8) {
            if (processFrame()) {
                _masterInfo.baudRate = BAUD_RATES[_baudIndex];
                updateMasterInfo();
                updateTimingStats();
                _lastFrameTime = millis();
                _bufferIndex = 0;
            } else {
                _masterInfo.invalidFrames++;
            }
        }
    }
    
    if (millis() - _lastActivityTime > 1000) {
        changeBaudRate();
        _lastActivityTime = millis();
    }
}

bool ModbusAnalyzer::isAnalyzing() const {
    return _analyzing;
}

unsigned long ModbusAnalyzer::getCurrentBaudRate() const {
    return BAUD_RATES[_baudIndex];
}

void ModbusAnalyzer::changeBaudRate() {
    _baudIndex = (_baudIndex + 1) % BAUD_COUNT;
    clearBuffer();
    _serial.end();
    delay(10);
    _serial.begin(BAUD_RATES[_baudIndex]);
    Serial.print(F("\nZmiana predkosci na: "));
    Serial.print(BAUD_RATES[_baudIndex]);
    Serial.println(F(" baud"));
}

void ModbusAnalyzer::updateTimingStats() {
    uint32_t currentTime = millis();
    if (_masterInfo.queryCount > 0) {
        uint32_t interval = currentTime - _masterInfo.lastQueryTime;
        
        if (interval < _masterInfo.minQueryInterval)
            _masterInfo.minQueryInterval = interval;
            
        if (interval > _masterInfo.maxQueryInterval)
            _masterInfo.maxQueryInterval = interval;
            
        _masterInfo.totalQueryTime += interval;
    }
    
    _masterInfo.lastQueryTime = currentTime;
    _masterInfo.queryCount++;
}

void ModbusAnalyzer::checkCollision() {
    uint32_t currentTime = millis();
    uint32_t timeSinceLastFrame = currentTime - _lastFrameTime;
    
    if (timeSinceLastFrame < COLLISION_THRESHOLD) {
        _masterInfo.collisions++;
        Serial.print(F("\n!!! Wykryto kolizje - "));
        Serial.print(timeSinceLastFrame);
        Serial.println(F("ms od ostatniej ramki"));
    }
}

bool ModbusAnalyzer::processFrame() {
    uint16_t receivedCrc = (_buffer[_bufferIndex-1] << 8) | _buffer[_bufferIndex-2];
    uint16_t calculatedCrc = calculateCRC16(_buffer, _bufferIndex-2);
    
    if (receivedCrc != calculatedCrc) {
        _masterInfo.crcErrors++;
        return false;
    }
    
    Serial.print(F("\nWykryto ramke Modbus:"));
    Serial.print(F("\nAdres: "));
    Serial.print(_buffer[0]);
    Serial.print(F(" Funkcja: 0x"));
    Serial.print(_buffer[1], HEX);
    Serial.print(F(" Predkosc: "));
    Serial.print(BAUD_RATES[_baudIndex]);
    Serial.println(F(" baud"));
    
    Serial.print(F("Ramka HEX:"));
    for (uint8_t i = 0; i < _bufferIndex; i++) {
        Serial.print(F(" "));
        if (_buffer[i] < 0x10) Serial.print('0');
        Serial.print(_buffer[i], HEX);
    }
    Serial.println();
    
    return true;
}

void ModbusAnalyzer::updateMasterInfo() {
    if (!isAddressInList(_buffer[0]) && _masterInfo.addressCount < 10) {
        _masterInfo.slaveAddresses[_masterInfo.addressCount++] = _buffer[0];
    }
    
    if (!isFunctionInList(_buffer[1]) && _masterInfo.functionCount < 5) {
        _masterInfo.functions[_masterInfo.functionCount++] = _buffer[1];
    }
    
    _masterInfo.startRegister = (_buffer[2] << 8) | _buffer[3];
    _masterInfo.registerCount = (_buffer[4] << 8) | _buffer[5];
}

void ModbusAnalyzer::clearBuffer() {
    while (_serial.available()) {
        _serial.read();
    }
    _bufferIndex = 0;
}

bool ModbusAnalyzer::isAddressInList(uint8_t address) const {
    for (uint8_t i = 0; i < _masterInfo.addressCount; i++) {
        if (_masterInfo.slaveAddresses[i] == address) return true;
    }
    return false;
}

bool ModbusAnalyzer::isFunctionInList(uint8_t function) const {
    for (uint8_t i = 0; i < _masterInfo.functionCount; i++) {
        if (_masterInfo.functions[i] == function) return true;
    }
    return false;
}

uint16_t ModbusAnalyzer::calculateCRC16(uint8_t* buffer, uint8_t length) const {
    uint16_t crc = 0xFFFF;
    for (uint8_t pos = 0; pos < length; pos++) {
        crc ^= (uint16_t)buffer[pos];
        for (uint8_t i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void ModbusAnalyzer::showSummary() const {
    Serial.println(F("\n=== Podsumowanie analizy Mastera ==="));
    
    // Podstawowe informacje
    Serial.print(F("Predkosc transmisji: "));
    Serial.print(_masterInfo.baudRate);
    Serial.println(F(" baud"));
    
    Serial.println(F("\nOdpytywane adresy:"));
    for (uint8_t i = 0; i < _masterInfo.addressCount; i++) {
        Serial.print(F("  ID: "));
        Serial.println(_masterInfo.slaveAddresses[i]);
    }
    
    Serial.println(F("\nUzywane funkcje:"));
    for (uint8_t i = 0; i < _masterInfo.functionCount; i++) {
        Serial.print(F("  0x"));
        Serial.print(_masterInfo.functions[i], HEX);
        Serial.print(F(" - "));
        Serial.println(getFunctionName(_masterInfo.functions[i]));
    }
    
    Serial.print(F("\nRejestr poczatkowy: "));
    Serial.println(_masterInfo.startRegister);
    Serial.print(F("Liczba rejestrow: "));
    Serial.println(_masterInfo.registerCount);
    
    // Statystyki czasowe
    Serial.println(F("\nStatystyki czasowe:"));
    if (_masterInfo.queryCount > 0) {
        Serial.print(F("Minimalny odstep: "));
        Serial.print(_masterInfo.minQueryInterval);
        Serial.println(F(" ms"));
        Serial.print(F("Maksymalny odstep: "));
        Serial.print(_masterInfo.maxQueryInterval);
        Serial.println(F(" ms"));
        Serial.print(F("Sredni odstep: "));
        Serial.print(_masterInfo.totalQueryTime / _masterInfo.queryCount);
        Serial.println(F(" ms"));
    }
    
    // Statystyki błędów
    Serial.println(F("\nStatystyki bledow:"));
    Serial.print(F("Ramki odebrane: "));
    Serial.println(_masterInfo.totalFrames);
    Serial.print(F("Bledy CRC: "));
    Serial.println(_masterInfo.crcErrors);
    Serial.print(F("Wykryte kolizje: "));
    Serial.println(_masterInfo.collisions);
    Serial.print(F("Nieprawidlowe ramki: "));
    Serial.println(_masterInfo.invalidFrames);
    Serial.println(F("==============================="));
}

const char* ModbusAnalyzer::getFunctionName(uint8_t function) const {
    switch (function) {
        case 0x01: return "Read Coils";
        case 0x02: return "Read Discrete Inputs";
        case 0x03: return "Read Holding Registers";
        case 0x04: return "Read Input Registers";
        case 0x05: return "Write Single Coil";
        case 0x06: return "Write Single Register";
        case 0x0F: return "Write Multiple Coils";
        case 0x10: return "Write Multiple Registers";
        default: return "Unknown Function";
    }
}