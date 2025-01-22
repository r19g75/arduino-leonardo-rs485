#ifndef MODBUS_SCANNER_H
#define MODBUS_SCANNER_H
#include <Arduino.h>

struct DeviceInfo {
    uint8_t address;
    unsigned long baudRate;
};

class ModbusScanner {
public:
    ModbusScanner(HardwareSerial& modbusSerial, uint8_t dirPin);
    void begin();
    void startScan();
    void stopScan();
    void update();
    bool isScanning() const;
    uint8_t getFoundDevicesCount() const;
    const DeviceInfo* getFoundDevices() const;
    bool readRegisters(uint8_t deviceAddr);

private:
    static const uint8_t MAX_FRAME_SIZE = 32;
    static const uint8_t MAX_DEVICES = 10;
    static const uint8_t MAX_MODBUS_ADDRESS = 247;
    static const unsigned long BAUD_RATES[];
    static const uint8_t BAUD_COUNT;

    HardwareSerial& _serial;
    uint8_t _dirPin;
    bool _scanning;
    uint8_t _currentAddress;
    uint8_t _currentBaudIndex;
    uint8_t _foundDevices;
    DeviceInfo* _devices;
    uint8_t _buffer[MAX_FRAME_SIZE];
    uint8_t _bufferIndex;
    uint32_t _lastActivityTime;

    bool testDevice(uint8_t address);
    void changeBaudRate();
    uint16_t calculateCRC16(uint8_t* buffer, uint8_t length) const;
    void clearBuffer();
};
#endif