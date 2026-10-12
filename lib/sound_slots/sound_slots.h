#ifndef SOUND_SLOTS_H
#define SOUND_SLOTS_H

/*
 * lib/sound_slots  –  ESP32-RC-Sound (host-seitiger Spiegel)
 *
 * Nach dem gleichen Muster wie lib/port_registry (siehe dort fuer die
 * ausfuehrliche Begruendung): eine reine, hardwareunabhaengige Kopie der
 * CRSF-Feld-ID-Vergabe rund um die Sound-Slots, damit sie per Unity unter
 * dem PlatformIO "native"-Environment getestet werden kann (test/test_
 * sound_slots), OHNE ESP32-Hardware.
 *
 * Artefakt eines zurueckgebauten Versuchs, RCSOUND_NUM_SLOTS (config.h) auf
 * 32 zu erweitern (siehe ESP32-RC-Sound.ino) - kSoundCount=32 hier passt
 * daher NICHT zum aktuellen RCSOUND_NUM_SLOTS-1 (24) in config.h. Diese
 * Datei ist nicht Teil des esp32dev-Builds und wird nur noch vom
 * "native"-Test-Environment eingebunden; sie bleibt bewusst unangetastet
 * liegen, statt sie zu loeschen oder synchron zu halten.
 *
 * Eine Erhoehung von RCSOUND_NUM_SLOTS verschiebt in ESP32-RC-Sound.ino
 * (crsfSendParam()/crsfWriteParam()) den kompletten "Einstellungen"-Block
 * (RC-System, Modul-Adresse, Gruppenadressen, Port B/C, Geraetename) um
 * eine feste Anzahl Feld-IDs nach hinten - das sind gut zwei Dutzend von
 * Hand zu verschiebende Literale, plus ggf. eigene Schleifengrenzen in
 * loop()/Config(), die KEINE Feld-ID-Literale sind und deshalb von diesem
 * Modul nicht abgedeckt werden - das bleibt Aufgabe eines vollstaendigen
 * Greps ueber ESP32-RC-Sound.ino vor jeder Aenderung. Was dieses Modul aber
 * zuverlaessig absichert: dass die Feld-ID-Arithmetik selbst (Sound-Formel,
 * Grenze zum Einstellungen-Block, CRSF_PARAM_COUNT) fuer die im Modul
 * gewaehlte Slot-Anzahl konsistent ist - ein Zahlendreher beim Verschieben
 * faellt hier sofort auf, statt erst auf dem echten Geraet.
 *
 * Manuell synchron zu haltende KOPIE der Werte aus config.h
 * (RCSOUND_NUM_SLOTS) und ESP32-RC-Sound.ino (crsfSendParam()/
 * crsfWriteParam(), CRSF_PARAM_COUNT) - kein gemeinsam genutzter Code, weil
 * .ino-Dateien nicht fuer das "native"-Environment mitkompiliert werden und
 * config.h <Arduino.h> zieht, das es auf dem Host nicht gibt. Bei einer
 * Aenderung von RCSOUND_NUM_SLOTS MUESSTE kSoundCount hier von Hand
 * nachgezogen werden (siehe test_sound_slots_count_matches_config_h_comment
 * in test_main.cpp als Erinnerung).
 */

namespace sound_slots {

// Anzahl der "echten" Sound-Slots OHNE den Motor-Slot (0) - entspricht
// RCSOUND_NUM_SLOTS-1 in config.h. Muss dort manuell synchron gehalten
// werden.
static constexpr int kSoundCount = 32;

// CRSF-Feld-ID-Vergabe (siehe Parameter-Doku ueber crsfSendParam() in
// ESP32-RC-Sound.ino): Sound s (1..kSoundCount):
//   fi = kFirstFieldId + (s-1)*kFieldsPerSound, belegt fi..fi+5.
static constexpr int kFirstFieldId   = 16;
static constexpr int kFieldsPerSound = 6;

constexpr int soundFolderId(int s) { return kFirstFieldId + (s - 1) * kFieldsPerSound; }
constexpr int lastSoundFieldId()   { return soundFolderId(kSoundCount) + kFieldsPerSound - 1; }

// "Einstellungen"-Block (Folder + RC-System, Modul-Adresse, Einkanal-Kanal,
// Einkanal-Mode, Hardware-Config, PWM min/max, 3 Gruppenadressen, Geraete-
// name-Info, Port-B-Modus, Port-B-Baudrate, Port-C/GPS) direkt nach dem
// letzten Sound-Feld = 15 aufeinanderfolgende Feld-IDs.
static constexpr int kSettingsBlockFieldCount = 15;

constexpr int settingsFolderId() { return lastSoundFieldId() + 1; }
constexpr int paramCount()       { return settingsFolderId() + kSettingsBlockFieldCount - 1; }

// Compile-Zeit-Pruefung (kein Testlauf noetig - greift automatisch, sobald
// diese Datei irgendwo eingebunden wird): sichert die fuer kSoundCount=32
// in diesem Modul verwendeten Feld-IDs ab. Wird kSoundCount hier geaendert,
// OHNE die Werte unten nachzuziehen, bricht der Build.
static_assert(soundFolderId(1) == 16,  "erstes Sound-Feld muss 16 sein");
static_assert(lastSoundFieldId() == 207, "letztes Sound-Feld passt nicht zu kSoundCount=32");
static_assert(settingsFolderId() == 208, "Einstellungen-Folder muss direkt nach dem letzten Sound-Feld liegen");
static_assert(paramCount() == 222, "CRSF_PARAM_COUNT in ESP32-RC-Sound.ino muss hierzu passen");

} // namespace sound_slots

#endif // SOUND_SLOTS_H
