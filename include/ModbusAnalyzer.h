#ifndef MODBUS_ANALYZER_H
#define MODBUS_ANALYZER_H

#include <Arduino.h>

struct MasterInfo {
    uint8_t slaveAddresses[10];     // Lista odpytywanych adresów
    uint8_t addressCount;           // Liczba różnych adresów
    unsigned long baudRate;         // Wykryta prędkość
    uint8_t functions[5];           // Lista używanych funkcji
    uint8_t functionCount;          // Liczba różnych funkcji
    uint16_t startRegister;         // Początkowy rejestr
    uint16_t registerCount;         // Liczba odczytywanych rejestrów
    
    // Statystyki czasowe
    uint32_t lastQueryTime;         // Czas ostatniego zapytania
    uint32_t minQueryInterval;      // Minimalny odstęp
    uint32_t maxQueryInterval;      // Maksymalny odstęp
    uint32_t totalQueryTime;        // Suma wszystkich odstępów
    uint32_t queryCount;            // Licznik zapytań (do średniej)
    
    // Statystyki błędów
    uint32_t totalFrames;           // Wszystkie odebrane ramki
    uint32_t crcErrors;             // Błędy CRC
    uint32_t collisions;            // Wykryte kolizje
    uint32_t invalidFrames;         // Nieprawidłowe ramki
};

class ModbusAnalyzer {
public:
    ModbusAnalyzer(HardwareSerial& modbusSerial);
    void begin();
    void startAnalysis();
    void stop();
    void update();
    bool isAnalyzing() const;
    unsigned long getCurrentBaudRate() const;
    void showSummary() const;

private:
    static const uint8_t MAX_BUFFER = 32;
    static const unsigned long BAUD_RATES[];
    static const uint8_t BAUD_COUNT;
    static const uint32_t COLLISION_THRESHOLD = 5;  // ms między ramkami = kolizja

    HardwareSerial& _serial;
    bool _analyzing;
    uint8_t _baudIndex;
    uint8_t _buffer[MAX_BUFFER];
    uint8_t _bufferIndex;
    uint32_t _lastActivityTime;
    uint32_t _lastFrameTime;
    MasterInfo _masterInfo;
    
    void changeBaudRate();
    uint16_t calculateCRC16(uint8_t* buffer, uint8_t length) const;
    bool processFrame();
    void clearBuffer();
    void updateMasterInfo();
    bool isAddressInList(uint8_t address) const;
    bool isFunctionInList(uint8_t function) const;
    const char* getFunctionName(uint8_t function) const;
    void updateTimingStats();
    void showTimingStats() const;
    void showErrorStats() const;
    void checkCollision();
};

#endif