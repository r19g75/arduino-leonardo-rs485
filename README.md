arduino-leonardo-rs485
Modbus Scanner & Analyzer
Opis
Narzędzie diagnostyczne do sieci Modbus RTU na RS485. Umożliwia:

Skanowanie i identyfikację urządzeń slave
Analizę zapytań mastera
Wykrywanie błędów komunikacji i kolizji
Pomiar czasów między zapytaniami

Wymagania sprzętowe

Arduino Leonardo
Shield RS485 (np. DFRobot RS485 Shield)
3 przyciski (dla pinów 8, 9, 10)
LED (wbudowana pin 13)

Schemat podłączenia
Arduino Leonardo:
- PIN 2  -> RS485 DIR
- PIN 8  -> Przycisk MASTER
- PIN 9  -> Przycisk SCAN
- PIN 10 -> Przycisk STOP
- PIN 13 -> Wbudowana LED

Instalacja

Sklonuj repozytorium
Otwórz projekt w PlatformIO
Wgraj kod do Arduino Leonardo

Użycie

SCAN (PIN 9): Skanowanie slave'ów, odczyt 10 pierwszych rejestrów
MASTER (PIN 8): Analiza zapytań mastera, statystyki błędów
STOP (PIN 10): Zatrzymanie i wyświetlenie raportu

Funkcje

Wykrywanie urządzeń Modbus
Odczyt rejestrów
Analiza ramek mastera
Statystyki czasowe komunikacji
Wykrywanie kolizji i błędów CRC

Rozwiązywanie problemów

Problem z komunikacją: Sprawdź prędkość transmisji
Brak odpowiedzi: Sprawdź połączenie RS485 (A/B)
Błędy CRC: Sprawdź terminację linii

TODO
Obsługa innych funkcji Modbus
Interfejs konfiguracyjny
Zapis logów na SD
Wykrywanie parametrów parzystości

Licencja
MIT License
Autor
Robert
Dodatkowe informacje
Projekt stworzony jako narzędzie diagnostyczne do sieci Modbus RTU. Pomocny przy uruchamianiu i diagnostyce problemów komunikacyjnych.
