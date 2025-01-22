#include "ModbusScanner.h"

const unsigned long ModbusScanner::BAUD_RATES[] = {9600, 19200, 38400, 57600, 115200};
const uint8_t ModbusScanner::BAUD_COUNT = sizeof(BAUD_RATES) / sizeof(BAUD_RATES[0]);

ModbusScanner::ModbusScanner(HardwareSerial& modbusSerial, uint8_t dirPin)
    : _serial(modbusSerial)
    , _dirPin(dirPin)
    , _scanning(false)
    , _currentAddress(1)
    , _currentBaudIndex(0)
    , _foundDevices(0)
    , _bufferIndex(0)
    , _lastActivityTime(0)
{
    _devices = new DeviceInfo[MAX_DEVICES];
}

void ModbusScanner::begin() {
    pinMode(_dirPin, OUTPUT);
    digitalWrite(_dirPin, LOW);
    _serial.begin(BAUD_RATES[0]);
}

void ModbusScanner::startScan() {
    _scanning = true;
    _currentAddress = 1;
    _currentBaudIndex = 0;
    _foundDevices = 0;
    _bufferIndex = 0;
    _lastActivityTime = millis();
    _serial.begin(BAUD_RATES[0]);
    clearBuffer();
    Serial.println(F("Start skanowania..."));
}

void ModbusScanner::stopScan() {
    _scanning = false;
    digitalWrite(_dirPin, LOW);
    Serial.println(F("Skanowanie zatrzymane"));
}

void ModbusScanner::update() {
    if (!_scanning) return;

    if (testDevice(_currentAddress)) {
        if (_foundDevices < MAX_DEVICES) {
            _devices[_foundDevices].address = _currentAddress;
            _devices[_foundDevices].baudRate = BAUD_RATES[_currentBaudIndex];
            
            Serial.print(F("\n*** Znaleziono urzadzenie "));
            Serial.print(_foundDevices + 1);
            Serial.println(F(" ***"));
            Serial.print(F("Adres: "));
            Serial.print(_currentAddress);
            Serial.print(F(", Predkosc: "));
            Serial.print(BAUD_RATES[_currentBaudIndex]);
            Serial.println(F(" baud"));
            
            // Odczyt rejestrów po znalezieniu urządzenia
            readRegisters(_currentAddress);
            
            _foundDevices++;
        }
    }

    _currentAddress++;
    if (_currentAddress > MAX_MODBUS_ADDRESS) {
        _currentAddress = 1;
        _currentBaudIndex++;
        if (_currentBaudIndex >= BAUD_COUNT) {
            _scanning = false;
            Serial.println(F("\nSkanowanie zakonczone"));
        } else {
            changeBaudRate();
        }
    }
}

bool ModbusScanner::isScanning() const {
    return _scanning;
}

uint8_t ModbusScanner::getFoundDevicesCount() const {
    return _foundDevices;
}

const DeviceInfo* ModbusScanner::getFoundDevices() const {
    return _devices;
}

void ModbusScanner::changeBaudRate() {
    clearBuffer();
    _serial.end();
    delay(10);
    _serial.begin(BAUD_RATES[_currentBaudIndex]);
    Serial.print(F("\nZmiana predkosci na: "));
    Serial.print(BAUD_RATES[_currentBaudIndex]);
    Serial.println(F(" baud"));
}

bool ModbusScanner::testDevice(uint8_t address) {
    digitalWrite(_dirPin, HIGH);
    delay(1);

    uint8_t query[] = {address, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
    uint16_t crc = calculateCRC16(query, 6);
    query[6] = crc & 0xFF;
    query[7] = (crc >> 8) & 0xFF;

    _serial.write(query, 8);
    _serial.flush();
    digitalWrite(_dirPin, LOW);
    delay(20);

    _bufferIndex = 0;
    uint32_t startTime = millis();

    while ((millis() - startTime) < 100 && _bufferIndex < MAX_FRAME_SIZE) {
        if (_serial.available()) {
            _buffer[_bufferIndex++] = _serial.read();
            if (_bufferIndex >= 5) {
                if (_buffer[0] == address && _buffer[1] == 0x03) {
                    clearBuffer();
                    return true;
                }
            }
        }
    }

    clearBuffer();
    return false;
}

bool ModbusScanner::readRegisters(uint8_t deviceAddr) {
    digitalWrite(_dirPin, HIGH);
    delay(1);
    
    uint8_t query[] = {deviceAddr, 0x03, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00};
    uint16_t crc = calculateCRC16(query, 6);
    query[6] = crc & 0xFF;
    query[7] = (crc >> 8) & 0xFF;
    
    _serial.write(query, 8);
    _serial.flush();
    digitalWrite(_dirPin, LOW);
    delay(100);

    _bufferIndex = 0;
    uint32_t startTime = millis();
    
    while ((millis() - startTime) < 200 && _bufferIndex < MAX_FRAME_SIZE) {
        if (_serial.available()) {
            _buffer[_bufferIndex++] = _serial.read();
        }
    }
    
    if (_bufferIndex >= 5 && _buffer[0] == deviceAddr && _buffer[1] == 0x03) {
        Serial.println(F("\nRejestr | Wartosc"));
        Serial.println(F("------------------"));
        
        for (uint8_t i = 0; i < 10; i++) {
            uint16_t value = (_buffer[3 + i*2] << 8) | _buffer[4 + i*2];
            Serial.print(i);
            Serial.print(F("\t"));
            Serial.print(value);
            Serial.print(F("\t(0x"));
            if (value < 0x1000) Serial.print('0');
            if (value < 0x100) Serial.print('0');
            if (value < 0x10) Serial.print('0');
            Serial.print(value, HEX);
            Serial.println(F(")"));
        }
        clearBuffer();
        return true;
    }

    clearBuffer();
    return false;
}

uint16_t ModbusScanner::calculateCRC16(uint8_t* buffer, uint8_t length) const {
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

void ModbusScanner::clearBuffer() {
    while (_serial.available()) {
        _serial.read();
    }
    _bufferIndex = 0;
}