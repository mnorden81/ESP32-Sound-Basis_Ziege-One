#ifndef PORT_REGISTRY_H
#define PORT_REGISTRY_H

/*
 * lib/port_registry  –  ESP32-RC-Sound (host-seitiger Spiegel)
 *
 * Neben der eigentlichen Firmware (src/, Arduino/ESP32-Hardware-abhaengig)
 * gibt es unter lib/ einen zweiten, bewusst hardwareunabhaengigen Satz
 * "Spiegel"-Module, die per Unity unter einem PlatformIO "native"-
 * Environment (siehe platformio.ini) auf dem Entwicklungsrechner getestet
 * werden - ganz ohne ESP32-Board, in Sekunden statt Minuten.
 *
 * Dieses Modul spiegelt NUR die reine, hardwareunabhaengige Entscheidungs-
 * logik der Port-B-Rollen-Registry (siehe src/ESP32-RC-Sound/port_function.h
 * und die Baudraten-Listen-Logik in ESP32-RC-Sound.ino):
 *   - PortBMode-Enum + PORTB_FUNCTION_COUNT + Namensliste + Options-String
 *   - clampPortBMode()
 *   - die kuratierte CRSF/Lua-Baudraten-Liste + portBBaudToIndex()
 *     (naechstgelegener Eintrag bei einem frei ueber's Web eingegebenen,
 *     nicht in der Liste enthaltenen Wert)
 *
 * Bewusst in Kauf genommenes Risiko: dies ist eine manuell synchron zu
 * haltende KOPIE der Logik aus port_function.cpp/ESP32-RC-Sound.ino, kein
 * gemeinsam genutzter Code - .ino-Dateien werden von PlatformIO nicht fuer
 * das "native"-Environment mitkompiliert, und die echten Module ziehen
 * <Arduino.h>/HardwareSerial* usw. mit, die es auf dem Host nicht gibt. Bei
 * einer Aenderung an einer der beiden Stellen (Namen, Reihenfolge,
 * Baudraten-Liste) MUSS die jeweils andere Stelle von Hand nachgezogen
 * werden - die Tests unter test/test_native pruefen nur, dass DIESE Kopie
 * in sich konsistent ist, nicht, dass sie mit der Firmware uebereinstimmt.
 */

#include <cstdint>

namespace port_registry {

enum PortBMode {
    PORTB_OFF   = 0,
    PORTB_CRSF  = 1,
    PORTB_SPORT = 2,
    PORTB_HOTT  = 3,   // in Vorbereitung
    PORTB_RAW   = 4
};

// Muss zu PORTB_FUNCTION_COUNT in src/ESP32-RC-Sound/config.h passen.
constexpr uint8_t PORTB_FUNCTION_COUNT = 5;

// Muss zu PORTB_MODULES[].name in src/ESP32-RC-Sound/port_function.cpp passen
// (Reihenfolge == numerischer PortBMode-Wert).
const char* const* portBFunctionNames();

// Anzeigename einer Rolle (auch fuer PORTB_OFF und ungueltige Werte: "Aus").
const char* portBFunctionName(PortBMode f);

// ";"-getrennte Namensliste, z.B. "Aus;CRSF;S.Port;Hott (bald);Raw" - muss zu
// portBFunctionOptionsString() in port_function.cpp passen.
const char* portBFunctionOptionsString();

// Groesster gueltiger Auswahl-Index (PORTB_FUNCTION_COUNT - 1).
constexpr uint8_t portBFunctionMaxIndex() { return PORTB_FUNCTION_COUNT - 1; }

// Auf einen gueltigen PortBMode begrenzt (unbekannte/alte NVS-Werte -> Aus).
// Muss zu clampPortBMode() in port_function.cpp passen.
PortBMode clampPortBMode(int raw);

// ── Kuratierte Port-B-Baudraten-Liste fuers CRSF/Lua-Menue ─────────────
// Muss zu PORTB_BAUD_LIST_STR/portBBaudList[]/PORTB_BAUD_LIST_COUNT/
// portBBaudToIndex() in src/ESP32-RC-Sound/ESP32-RC-Sound.ino passen.
constexpr uint8_t PORTB_BAUD_LIST_COUNT = 7;
const uint32_t* portBBaudList();
const char* portBBaudListString();

// Index des exakten Treffers, sonst des naechstgelegenen Eintrags (Snapping
// fuer frei ueber's Web eingegebene Werte, die nicht in der Liste stehen).
uint8_t portBBaudToIndex(uint32_t baud);

} // namespace port_registry

#endif // PORT_REGISTRY_H
