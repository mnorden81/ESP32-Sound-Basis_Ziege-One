#ifndef SOUND_MAP_H
#define SOUND_MAP_H

/*
 * sound_map.h  -  ESP32-RC-Sound, Sound-Zuordnung (v7.24)
 *
 * Bis v7.23 hiess jede Datei fest loop.wav, shut.wav, start.wav,
 * sound1.wav ... sound24.wav, und der Name bestimmte den Platz. Seit v0.12
 * behaelt jede hochgeladene Datei ihren eigenen Namen (Umlaute und
 * Leerzeichen ersetzt), und config.Sound_File[] haelt fest, welche Datei
 * auf welchem Platz spielt. Dieselbe Datei darf auf mehreren Plaetzen
 * liegen.
 *
 * Plaetze (Index in config.Sound_File[]):
 *   0 = Loop, 1 = Standgas, 2 = Start, 3.. = Sound 1-24
 * Schluessel in Web/JSON: "loop", "shut", "start", "1" ... "24".
 *
 * Alte Module behalten ihre Dateien: ohne gespeicherte Zuordnung gelten die
 * festen Namen von frueher (soundMapDefaultName()).
 */

#include <Arduino.h>
#include "config.h"

class XT_Wav_Class;

// Feste Namen von frueher (Werkseinstellung): loop.wav ... sound4.wav
void soundMapDefaultName(int slot, char* out);

// Anzeigename des Platzes: "Loop", "Standgas", "Start", "Sound 3"
String soundMapSlotLabel(int slot);
// Schluessel fuer Web/JSON: "loop", "shut", "start", "3"
String soundMapSlotKey(int slot);
// Schluessel -> Platz, -1 = unbekannt
int    soundMapSlotFromKey(const String& key);

// Dateiname gueltig? strict = Regel fuer neu hochgeladene Dateien
// (nur A-Z a-z 0-9 _ -, Name hoechstens SND_NAME_MAX_BASE Zeichen + ".wav").
// strict = false: auch vorhandene Dateien mit anderen Zeichen (nur kein
// "/", "\", ";", Anfuehrungszeichen oder Steuerzeichen), Endung .wav in
// beliebiger Schreibweise.
bool   soundNameOk(const char* name, bool strict);

// Sound-Objekt eines Platzes (Sound_loop, ..., Sound1 ...)
XT_Wav_Class* soundMapObject(int slot);

// Dateinamen aus config.Sound_File[] in die Sound-Objekte uebernehmen.
// Einmal in setup() vor LoadFiles() aufrufen.
void   soundMapApplyAll();

// Platz neu belegen ("" = leer). Haelt einen laufenden Sound an, laedt die
// Motor-Dateien sofort neu und markiert die Konfiguration zum Speichern.
// false = ungueltiger Platz oder Name.
bool   soundMapSet(int slot, const char* name);

// Vor Upload/Loeschen einer Datei: alle Plaetze mit dieser Datei anhalten
// und schliessen. Nachher: Motor-Plaetze mit dieser Datei neu laden.
void   soundMapCloseUsers(const char* name);
void   soundMapReloadUsers(const char* name);
// Nach dem Loeschen: alle Plaetze mit dieser Datei leeren.
void   soundMapClearUsers(const char* name);

// Alle WAV-Dateien im Hauptordner der SD-Karte, alphabetisch. Liest ueber
// opendir()/stat() (oeffnet keine Datei) unter dem SD-Mutex. Rueckgabe = Anzahl.
static constexpr int SND_LIST_MAX = 48;
struct SoundFileInfo { char name[SND_FILE_LEN]; uint32_t size; };
int    soundListFiles(SoundFileInfo* out, int max);
// Groesse einer Datei, false = nicht vorhanden
bool   soundFileSize(const char* name, uint32_t& size);

#endif
