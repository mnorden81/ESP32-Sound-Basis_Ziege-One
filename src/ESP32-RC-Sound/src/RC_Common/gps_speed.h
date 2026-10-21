/*
 * gps_speed.h  –  RC_Common (Rolle "GPS" auf einem Zusatz-UART, siehe
 * uart_roles.h; urspruenglich aus ESP32-RC-Sound v7)
 *
 * Liest ein NMEA-GPS-Modul (z. B. u-blox NEO-M8N oder ATGM336H) über einen
 * Hardware-UART ein und stellt Ground Speed sowie Positionsdaten bereit.
 *
 * Anschluss an einen Zusatz-UART mit Rolle "GPS" (Pins siehe Projekt):
 *   GPS-TX  -> Port RX  (Empfang der NMEA-Daten)
 *   GPS-RX  <- Port TX  (Senden der Konfiguration an das GPS)
 *
 * Nach dem Start wird das GPS einmalig per CASIC-Kommando auf 5 Hz gestellt
 * (ATGM336H/AT6558) und auf die Ausgabe von GGA+RMC reduziert. Die Einstellung
 * ist flüchtig und wird bei jedem Start neu gesetzt.
 *
 * Benötigte Bibliothek: TinyGPSPlus.
 */
#ifndef GPS_SPEED_H
#define GPS_SPEED_H

#include <Arduino.h>

void     gpsSpeedInit(HardwareSerial* serial, uint8_t rxPin, uint8_t txPin, uint32_t baud); // GPS am UART starten
// Bereits geoeffneter Datenstrom, z. B. SoftwareSerial (Soundmodul Basis Port C)
void     gpsSpeedInitStream(Stream* stream);
bool     gpsIsActive();        // true, sobald ein Port die Rolle GPS hat
void     gpsSpeedUpdate();     // NMEA einlesen/auswerten + Konfig nach Boot senden

float    gpsGetSpeedKmh();     // aktuelle Geschwindigkeit in km/h
uint8_t  gpsGetSatellites();   // Anzahl Satelliten im Fix
bool     gpsHasFix();          // gültiger Positions-Fix?

int32_t  gpsGetLatE7();        // Breitengrad * 1e7   (für CRSF-GPS-Frame)
int32_t  gpsGetLonE7();        // Längengrad * 1e7
uint16_t gpsGetHeadingCdeg();  // Kurs über Grund * 100
uint16_t gpsGetAltMeters();    // Höhe in Metern (Frame addiert +1000 selbst)

#endif // GPS_SPEED_H
