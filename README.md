# ESP32-RC-Sound V7

Soundmodul-Firmware für RC-Modelle auf ESP32-Basis. Spielt WAV-Dateien von einer SD-Karte über I2S ab, gesteuert über den RC-Bus (SBUS oder CRSF/ELRS), und lässt sich vollständig über eine eingebaute Weboberfläche oder direkt am Sender im CRSF-/Lua-Parametermenü konfigurieren.

Für Fahrzeuge, Baumaschinen, Panzer, Boote — überall dort, wo ein Modell klingen soll wie das Original.

Projektbeschreibung als Word-Dokument: [`docs/Projektbeschreibung.docx`](docs/Projektbeschreibung.docx)

---

## Projektvideo

[![Projektvideo: CRSF-Ökosystem für RC-Modelle (zum Abspielen anklicken)](https://github.com/mnorden81/RC-CRSF-Router-ESP32-S3/raw/main/docs/video/projektvideo-vorschau.jpg)](https://github.com/mnorden81/RC-CRSF-Router-ESP32-S3/blob/main/docs/video/CRSF-Oekosystem_Projektvorstellung.mp4)

Rund zwei Minuten Überblick über das ganze CRSF-Ökosystem: CRSF-Router, Multiswitch, Soundmodule, der ESP-NOW-CRSF-Link mit Funk in den Modulen und die Prüfstände für WebUI und Lua-Menü. Bild anklicken, dann startet das Video auf GitHub. Das Video liegt im Repo [RC-CRSF-Router-ESP32-S3](https://github.com/mnorden81/RC-CRSF-Router-ESP32-S3/tree/main/docs/video), weil es alle Module gemeinsam zeigt.

## Funktionen

- **Motorsound** mit Start-/Loop-/Stopp-Zyklus, Gasrampe und einstellbarer Standgas-Verzögerung; die Loop-Tonhöhe folgt dem Gaskanal
- **24 frei zuweisbare Zusatzsounds** (Hupe, Blinker, Licht, Sirene, Ladeklappe …), je Slot mit eigener Quelle, Lautstärke und Wiedergabemodus (Normal / Loop / Tippbetrieb)
- **RC-Systeme:** FrSky, FlySky, ELRS (SBUS), Hott, ELRS (CRSF)
- **Funk (ab v7.25):** Module ohne Kabel per ESP-NOW verbinden, gleich wie Multiswitch und CRSF-Router. RC-System „CRSF + Funk senden (Zentrale)“ am Empfänger oder „CRSF aus Funk (Teilnehmer)“, dann bleibt Port A frei. Web-Tab „Funk“ mit Verbindungsanzeige
- **Mehrfachadress-Erweiterung („MKan")** — 16 zusätzliche Einzelkanäle über zwei Gruppen, wahlweise per CRSF-Gruppenadresse oder per SBUS-Gruppenkanal
- **Telemetrie:** S.Port-LiPo-Zellüberwachung (FLVSS/MLVSS, 1–6S automatisch erkannt) und GPS-Geschwindigkeit
- **Weboberfläche** über eigenen Access Point: Konfiguration, Sound-Upload/-Löschen, Konfigurations-Backup als JSON oder auf SD-Karte
- **CRSF-/Lua-Parametermenü** — vollständige Bedienung am Sender ohne Handy oder PC (ab v7.25 ein Ordner „Sounds“ mit Auswahl „Sound Nr“ für alle 24 Sounds, dazu Ordner „Funk“)
- **OTA-Update** per WLAN, ohne Ausbau des Moduls
- **WLAN-Failsafe:** schaltet den AP automatisch ein, wenn längere Zeit kein gültiges RC-Signal anliegt

---

## Hardware

Getestet auf gängigen ESP32-DevKit-Boards (PlatformIO-Board `esp32dev`), mit SD-Kartenmodul und I2S-Verstärker (z. B. MAX98357A).

### Pin-Belegung

| Funktion | GPIO |
|---|---|
| WLAN-Aktivierung (LOW beim Booten = AP-Modus) | 13 |
| CRSF RX / SBUS RX | 16 |
| CRSF TX | 17 |
| SD: CS / CLK / MISO / MOSI | 5 / 18 / 19 / 23 |
| I2S: DOUT / LRC / BCLK | 21 / 25 / 26 |
| Port B — RX / TX (nur V3) | 32 / 33 |
| Port C — GPS (SoftwareSerial) | 27 / 14 |

### Hardware-Varianten

| Variante | Eingänge |
|---|---|
| **V1** | BUS + PWM-Eingang + GPIO-Pin + Einkanal (GPIO 22, 0, 2, 4) |
| **V2** | wie V1, andere Pins (GPIO 14, 27, 32, 33) |
| **V3** | nur BUS-Kanal + Einkanal, dafür Port B und Port C |

### Port-Modell (V3)

- **Port A** — RC-Eingang, fest belegt (CRSF oder SBUS-Familie)
- **Port B** — zweiter Hardware-UART, Rolle frei wählbar: Aus, S.Port-Master, CRSF (zweite Instanz), GPS (ab v7.20), Hott (Menü vorhanden, Treiber offen) oder Raw-UART, je mit eigener Baudrate
- **Port C** — GPS für CRSF-Telemetrie, unabhängig zuschaltbar. Nur GPS möglich: Port C läuft per SoftwareSerial, weil der ESP32 keinen dritten freien Hardware-UART hat. Steht Port B auf GPS, bleibt Port C aus.

> **Achtung:** Port C (GPS) und die SBUS-Familie belegen beide GPIO 27. Bei gleichzeitiger Aktivierung bleibt GPS deaktiviert und die Firmware meldet einen GPIO27-Konflikt. Für GPS das RC-System auf ELRS (CRSF) stellen.

S.Port-Verdrahtung an Port B: `GPIO33 →|1N4148|→ S.Port-Signal ← GPIO32`

---

## Sounds auf der SD-Karte

Dateien liegen im Wurzelverzeichnis der Karte. **Seit v7.24 behalten Sounds ihren Namen**, und du legst fest, welcher Sound auf welchem Platz spielt (Loop, Standgas, Start, Sound 1–24):

- **Web-UI, Tab „Speicher“:** alle WAV-Dateien der Karte mit Größe und Platz, pro Platz eine Auswahlliste. Ein Sound darf auf mehreren Plätzen liegen. Dort auch Hochladen (mehrere auf einmal) und Löschen.
- **Web-UI, Tabs „Motor“ und „Sounds“:** „Hochladen“ speichert die Datei mit ihrem Namen und legt sie gleich auf diesen Platz.
- **Lua-Menü, Ordner „Sound-Zuordnung“:** erst „Platz“ wählen, dann bei „Datei“ den Sound. Namen sind dort auf 12 Zeichen gekürzt, die Liste zeigt bis zu 20 Dateien.

Beim Hochladen werden Umlaute ersetzt (ä → ae), Leerzeichen werden zu `_`, Namen bis 40 Zeichen plus `.wav`. Per Kartenleser kopierte Dateien erscheinen im Tab „Speicher“ und müssen dort einem Platz zugeordnet werden. Ausnahme: Ohne eigene Zuordnung gelten wie bisher die festen Namen:

```
loop.wav  shut.wav  start.wav          Motorsound
sound1.wav … sound24.wav               Zusatzsounds 1–24
```

**Format:** 16-Bit-PCM-WAV, mono oder stereo, maximal 44.100 Hz.

Dateien lassen sich per Kartenleser kopieren oder direkt über die Weboberfläche hochladen und löschen — eine hochgeladene Datei wirkt sofort, ohne Neustart.

---

## Firmware flashen

Ein neues Modul wird einmal per USB mit der Arduino IDE geflasht (Weg 1). Danach geht jedes Update bequem per WLAN (Weg 2). Die Einstellungen bleiben bei beiden Wegen erhalten, die Sounds liegen ohnehin auf der SD-Karte.

### Weg 1: Arduino IDE (USB)

**Einmalig einrichten**

1. Arduino IDE 2.x installieren (https://www.arduino.cc/en/software).
2. **Datei → Einstellungen → Zusätzliche Boardverwalter-URLs**: `https://espressif.github.io/arduino-esp32/package_esp32_index.json` eintragen.
3. **Werkzeuge → Board → Boardverwalter**: „esp32“ suchen, **esp32 von Espressif Systems** in Version **3.0.7** installieren (neuere 3.x gehen auch).
4. **Werkzeuge → Bibliotheken verwalten**: **Bolder Flight Systems SBUS 8.1.4**, **EspSoftwareSerial 8.2.0** und **TinyGPSPlus 1.0.3** suchen und installieren.
   Alles andere (z. B. `src/RC_Common/`) liegt schon im Sketch-Ordner.

**Flashen**

1. Repo herunterladen: oben auf GitHub **Code → Download ZIP**, entpacken (oder den vorhandenen Ordner nehmen).
2. In der Arduino IDE **Datei → Öffnen** und `src/ESP32-RC-Sound/ESP32-RC-Sound.ino` wählen. Die Ordner `lib/` und `test/` **nicht** in den Sketch-Ordner kopieren (sonst „multiple definition of …“).
3. Unter **Werkzeuge** einstellen:

   | Einstellung | Wert |
   |---|---|
   | Board | **ESP32 Dev Module** |
   | Flash Size | 4MB (32Mb) |
   | Partition Scheme | **Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)** (OTA-fähig, wie GitHub) |
   | Erase All Flash Before Sketch Upload | **Disabled** (sonst sind die Einstellungen weg) |

4. ESP32-Board mit einem USB-**Datenkabel** an den PC stecken. Windows braucht evtl. einen Treiber für den USB-Seriell-Chip auf dem Board (CP210x von Silicon Labs oder CH340). Dann unter **Werkzeuge → Port** den neuen COM-Port wählen.
5. Auf den Pfeil **Hochladen** klicken. Am Ende steht „Hard resetting via RTS pin…“, das Modul startet neu.

**Wenn das Hochladen nicht klappt** („Failed to connect“): Sobald unten „Connecting…“ erscheint, die Taste **BOOT** (manchmal „IO0“) auf dem ESP32-Board gedrückt halten, bis die Prozentzahlen laufen. Dann noch einmal **Hochladen**.

### Weg 2: Update per WLAN (OTA)

1. Die Datei `.bin` besorgen:
   - aus GitHub: angemeldet oben auf **Actions** klicken, links den Workflow **CI** wählen, den obersten Lauf mit grünem Haken öffnen, unten unter **Artifacts** **ESP32-RC-Sound-firmware** herunterladen und entpacken. Darin liegt `firmware.bin`. Fehlt der Download (Artefakte werden nach 90 Tagen gelöscht): rechts oben **Run workflow** → **Run workflow**, nach ca. 3 Minuten ist ein neuer Lauf fertig.
   - oder in der Arduino IDE mit **Sketch → Kompilierte Binärdatei exportieren** (liegt dann im Sketch-Ordner unter `build/`, Datei `ESP32-RC-Sound.ino.bin`; die Dateien `…bootloader.bin` und `…partitions.bin` werden nicht gebraucht).
2. WLAN des Moduls einschalten (ab Werk „WLAN dauerhaft an“, sonst siehe [WLAN aktivieren](#wlan-aktivieren)). Name **ESP32-RC-Sound**, Passwort `123456789` (falls nicht geändert).
3. Mit Handy oder PC in dieses WLAN gehen und im Browser http://192.168.1.1 öffnen.
4. Tab **Einstellungen**, Karte **Firmware-Update (OTA)**: die `.bin`-Datei auswählen und hochladen. Das Modul startet danach neu.

Schlägt das Update fehl oder wird es abgebrochen, bleibt die bisherige Firmware aktiv und startfähig.

> **Praxistipp:** OTA möglichst mit über ESC/Akku bestromtem Modul durchführen. Manche USB-Anschlüsse liefern beim Flash-Schreiben nicht genug Strom; ein Brownout-Reset bricht den Upload ab.

GitHub baut für dieses Modul nur die Update-Datei, kein Komplett-Image. Ein ganz neues Modul geht deshalb nur über Weg 1.

### PlatformIO (für Entwickler)

```bash
pio run -e esp32dev              # Firmware bauen
pio run -e esp32dev -t upload    # bauen und flashen
pio test -e native               # Host-Tests, ohne ESP32-Hardware
```

Bibliotheksversionen sind in `platformio.ini` fest gepinnt. Die Partitionstabelle steht auf `default.csv` (Standard des Arduino-ESP32-Cores) — zwei App-Partitionen à 1,25 MB, damit OTA funktioniert.

---

## Konfiguration

### WLAN aktivieren

Drei Wege zum Access Point:

1. **GPIO 13 beim Einschalten kurz auf GND** — funktioniert immer, auch ganz ohne Sender
2. **Schalter im Web-Tab „WiFi"** bzw. Lua „WLAN jetzt“ — wirkt sofort, ohne Neustart
3. **Auto-Failsafe** — schaltet selbsttätig ein, wenn 5–240 s (Standard 60 s) kein gültiges RC-Signal anliegt

Der Failsafe schaltet **nur ein, nie automatisch wieder aus**. Das ist Absicht: sonst bräche eine laufende Konfiguration oder ein laufendes OTA-Update ab, sobald kurz wieder ein Signal hereinkommt.

### Am Sender

Bei RC-System ELRS (CRSF) ist das Modul vollständig über das native Parametermenü des Senders bedienbar, etwa als Lua-Script in EdgeTX. Jedes Modul bezieht seine CRSF-Busadresse (`0xC0` + WM-Adresse) und seinen Telemetrie-Zeitslot aus der WM-Adresse nach dem Wilhelm-Meier-Schema — mehrere Module am selben Bus stören sich dadurch nicht.

Ab v7.25 gibt es für die Zusatzsounds nur noch **einen Ordner „Sounds“**: oben „Sound Nr“ wählen (1–24), darunter erscheinen Name (zugeordnete Datei), Quelle, Kanal Nr, Volumen, Wiedergabe Mode und Test Sound dieses Sounds. Das ist in allen Soundmodulen gleich.

### Funk (ab v7.25)

- **Zentrale:** RC-System „CRSF + Funk senden (Zentrale)“. Das Modul hängt am Empfänger (Port A) und funkt die Kanäle an die Teilnehmer weiter. Pro Fahrzeug genau eine Zentrale.
- **Teilnehmer:** RC-System „CRSF aus Funk (Teilnehmer)“. Kanäle kommen nur aus dem Funk, Port A bleibt frei.
- Kennung (Fahrzeug) und Funkkanal müssen bei allen Modulen eines Fahrzeugs gleich sein. Einstellbar in der Web-UI (Einstellungen, Karte „Funk“) oder im Lua-Ordner „Funk“, wirkt nach einem Neustart.
- Bei Funkverlust schaltet der Teilnehmer nach 2 s auf Failsafe (Motor und Sounds aus).
- Der Access Point läuft mit Funk auf dem Funkkanal. WLAN bleibt technisch an (ESP-NOW braucht es), das kostet etwas RAM: nach dem ersten Test bitte „RAM frei“ in der Web-UI ansehen.

---

## Fehlerbehebung

| Symptom | Ursache | Lösung |
|---|---|---|
| Sound-Upload meldet `sd_locked`, Löschen geht auch nicht | Die WAV-Datei auf der Karte trägt das FAT-Attribut *schreibgeschützt*. Windows übernimmt es beim Kopieren oft unbemerkt — und nur für einzelne Dateien | Am PC prüfen mit `attrib X:\*.wav`, aufheben mit `attrib -r X:\*.wav` |
| Upload meldet `sd_write` | Karte voll, schreibgeschützt oder defekt | Serial-Log ansehen: die Firmware gibt bei jedem Fehlschlag freien Kartenspeicher, freigegebene Datei-Handles und freien Heap aus |
| Firmware startet, aber kein AP sichtbar | GPIO 13 beim Start nicht auf LOW | Taster/Brücke gegen GND beim Einschalten, oder Failsafe abwarten |
| GPS meldet „GPIO27-Konflikt" | Port C und SBUS gleichzeitig aktiv | RC-System auf ELRS (CRSF) stellen, GPS auf Port B legen oder GPS deaktivieren |
| Port B steht auf CRSF, aber nichts passiert | Die Modulkette (CRSF-Router) läuft nur, wenn Port A CRSF ist | RC-System auf ELRS (CRSF) stellen |
| „multiple definition of …" beim Kompilieren | `lib/` und `test/` wurden in den Sketch-Ordner kopiert | Nur den Inhalt von `src/` verwenden |

### Hinweis für Mitentwickler: `max_files` in `SD.begin()`

Der Standardwert (5 gleichzeitig offene Dateien) sieht knapp aus und lädt zum Anheben ein. **Nicht tun.** Ein höherer Wert belegt beim Mounten dauerhaft einen deutlich größeren Block im internen DRAM, und der verhindert, dass der WLAN-Treiber seine *zusammenhängenden* Puffer bekommt:

```
E wifi: Expected to init 4 rx buffer, actual is 3
E wifi: rc_enable_trc fail, no mem
```

Das tritt auf, obwohl über 100 KB Heap frei sind — es ist ein Fragmentierungs-, kein Platzproblem. Der Engpass an Datei-Handles wird stattdessen gelöst, indem während eines Uploads nicht benötigte Handles freigegeben und danach neu geladen werden (`releaseIdleSoundHandles()` in `WebServerManager.cpp`).

---

## Projektstruktur

```
src/ESP32-RC-Sound/
  ESP32-RC-Sound.ino     Hauptprogramm, Sound-Logik, CRSF-Parametermenü
  config.cpp/.h          Persistente Konfiguration (NVS), Migration
  WebServerManager.cpp   Weboberfläche, REST-API, OTA
  XT_I2S_Audio.cpp/.h    WAV-Wiedergabe über I2S, SD-Zugriff
  src/RC_Common/         Gemeinsame Basis aller ESP32-Module (identische Kopie,
                         CRSF mit Router, UART-Rollen, S.Port, GPS, Lua-Menü,
                         Einstellungen, WLAN-Failsafe, OTA; eigene Host-Tests)
lib/sound_slots/         Altes Artefakt, nur noch für test/test_sound_slots
test/                    Unity-Tests, laufen ohne ESP32
docs/                    Projektbeschreibung (Word)
```

`src/RC_Common/` darf in diesem Repo nicht einzeln geändert werden: die CI prüft die Kopie gegen `MANIFEST.sha256` und führt ihre Host-Tests aus (`tests/run_tests.sh`). Danach baut sie die Firmware und führt `pio test -e native` aus.

---

## Versionsstand

Aktuell: **v7.24**. Alle Versionen mit Änderungen: [CHANGELOG.md](CHANGELOG.md).

Dieses Repository enthält nur die V7-Linie. Ältere Stände (V5, V6) sind getrennte Projekte und liegen nicht hier.

---

## Herkunft und Dank

Die Hardware dieses Soundmoduls stammt von **Der RC-Modellbauer** (Frank Verfürth, auf GitHub **Ziege-One**).

- Webseite und Shop: [derrcmodellbauer.de](https://derrcmodellbauer.de/)
- Platinen: [ESP32 RC-Sound Platine V2](https://derrcmodellbauer.de/produkt/esp32-rc-sound-platine/), [ESP32 RC-Sound Platine V3](https://derrcmodellbauer.de/produkt/esp32-rc-sound-platine-v3/), [ESP32 RC-Sound Bausatz](https://derrcmodellbauer.de/produkt/esp32-rc-sound-bausatz/)
- GitHub: [github.com/Ziege-One](https://github.com/Ziege-One), Vorlagen [ESP32-RC-Sound](https://github.com/Ziege-One/ESP32-RC-Sound), [ESP32-SBus-Switch](https://github.com/Ziege-One/ESP32-SBus-Switch) und [CRSF_ESP32](https://github.com/Ziege-One/CRSF_ESP32)
- YouTube: [Der RC-Modellbauer](https://www.youtube.com/@DerRCModellbauer)

Technische Basis der Firmware ist sein Projekt *ESP32-SBUS-Switch* (2022) — von dort stammen der grundlegende Ansatz für SBUS-/CRSF-/S.Port-Protokoll, die WLAN-Weboberfläche und die I2S-Audioausgabe. Das Soundmodul ist daraus als eigenständiges, neu entworfenes Projekt hervorgegangen.

Die Audio-Engine (`XT_I2S_Audio`) stammt von **XTronical** und steht unter GNU GPL 3.0.

Das Adressierungsschema für mehrere Module am selben CRSF-Bus folgt dem Schema von **Wilhelm Meier**.

---

## Lizenz

Das Projekt steht unter der **GNU General Public License, Version 3 oder später** (GPL-3.0-or-later), siehe [LICENSE](LICENSE).

Grund: Die enthaltene Audio-Engine von XTronical (`XT_I2S_Audio`) steht unter GPL 3.0 oder später, die Vorlagen von Ziege-One (*ESP32-SBUS-Switch* und *CRSF_ESP32*) stehen unter GPL 3.0. Eine Firmware, die diesen Code enthält und weitergegeben wird, muss als Ganzes unter einer GPL-3.0-kompatiblen Lizenz stehen. Die eingebundenen Bibliotheken (Bolder Flight SBUS: MIT, EspSoftwareSerial und TinyGPSPlus: LGPL-2.1) sind damit vereinbar.

## Prüfstand (WebUI und Lua im Browser)

In [`pruefstand/`](pruefstand/) liegt eine Seite, die die echte Weboberfläche und das Lua-Menü dieser Firmware im Browser zeigt (alle Varianten, Modul simuliert): [Prüfstand öffnen](https://raw.githack.com/mnorden81/ESP32-Sound-Basis_Ziege-One/main/pruefstand/pruefstand.html). Sie wird bei jeder Firmware-Änderung neu gebaut, die GitHub-Prüfung „Prüfstand" achtet darauf.
