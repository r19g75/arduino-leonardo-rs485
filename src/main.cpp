#include <Arduino.h>
#include "ModbusScanner.h"
#include "ModbusAnalyzer.h"

const uint8_t RS485_DIR_PIN = 2;
const uint8_t MASTER_BUTTON = 8;    // Analiza mastera
const uint8_t SCAN_BUTTON = 9;      // Skanowanie slave
const uint8_t STOP_BUTTON = 10;     // Stop
const uint8_t LED_PIN = 13;

enum class Mode {
    IDLE,
    SCANNING,
    ANALYZING
} currentMode = Mode::IDLE;

ModbusScanner scanner(Serial1, RS485_DIR_PIN);
ModbusAnalyzer analyzer(Serial1);
unsigned long lastBlink = 0;
bool ledState = false;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(MASTER_BUTTON, INPUT_PULLUP);
    pinMode(SCAN_BUTTON, INPUT_PULLUP);
    pinMode(STOP_BUTTON, INPUT_PULLUP);
    pinMode(RS485_DIR_PIN, OUTPUT);
    digitalWrite(RS485_DIR_PIN, LOW);
    
    Serial.begin(115200);
    scanner.begin();
    analyzer.begin();
    
    Serial.println(F("\nModbus Scanner & Analyzer v1.0"));
    Serial.println(F("1. SCAN (PIN 9) - skanowanie slave"));
    Serial.println(F("2. MASTER (PIN 8) - nasłuchiwanie mastera"));
    Serial.println(F("3. STOP (PIN 10) - zatrzymanie i raport"));
}

void loop() {
    if (currentMode != Mode::IDLE) {
        unsigned long interval = (currentMode == Mode::SCANNING) ? 500 : 100;
        if (millis() - lastBlink >= interval) {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
            lastBlink = millis();
        }
    } else {
        digitalWrite(LED_PIN, LOW);
    }
    
    if (currentMode == Mode::IDLE) {
        if (digitalRead(SCAN_BUTTON) == LOW) {
            delay(50);
            if (digitalRead(SCAN_BUTTON) == LOW) {
                currentMode = Mode::SCANNING;
                scanner.startScan();
            }
        }
        
        if (digitalRead(MASTER_BUTTON) == LOW) {
            delay(50);
            if (digitalRead(MASTER_BUTTON) == LOW) {
                currentMode = Mode::ANALYZING;
                analyzer.startAnalysis();
            }
        }
    }
    
    if (digitalRead(STOP_BUTTON) == LOW) {
        delay(50);
        if (digitalRead(STOP_BUTTON) == LOW) {
            if (currentMode == Mode::SCANNING) {
                scanner.stopScan();
            } else if (currentMode == Mode::ANALYZING) {
                analyzer.stop();
                analyzer.showSummary();
            }
            currentMode = Mode::IDLE;
            digitalWrite(LED_PIN, LOW);
        }
    }
    
    switch (currentMode) {
        case Mode::SCANNING:
            scanner.update();
            if (!scanner.isScanning()) {
                currentMode = Mode::IDLE;
                digitalWrite(LED_PIN, LOW);
            }
            break;
            
        case Mode::ANALYZING:
            analyzer.update();
            break;
            
        default:
            break;
    }
}