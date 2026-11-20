# Versionsgeschichte ESP32-RC-Sound V7

Dieses Repository enthält nur die V7-Linie. Ältere Stände (V5, V6) sind getrennte Projekte und liegen nicht hier. Die Version wird bei jeder Änderung erhöht, weil sie als CRSF „Parameter Version Number“ an den Sender geht. Neueste Version oben, aufgeführt sind die wichtigen Schritte.

## v7.25 (RC_Common 1.5.2)
- Neu: Funk (ESP-NOW) wie Multiswitch v5.04 und CRSF-Router. RC-System 5 = „CRSF + Funk senden (Zentrale)“, 6 = „CRSF aus Funk (Teilnehmer)“, Port A bleibt dann frei. Web-UI: Karte „Funk“ in den Einstellungen und Tab „Funk“ mit Verbindungsanzeige. Lua: Ordner „Funk“.
- Lua: statt 24 Ordnern „Sound 1“ … „Sound 24“ ein Ordner „Sounds“ mit Auswahl „Sound Nr“ (gleich in allen Soundmodulen). Feld-IDs neu durchnummeriert, 57 statt 185 Felder.
- Noch nicht auf Hardware getestet. Flash 97,1 % (default.csv).

## v7.24
- Sound-Zuordnung: Sounds behalten beim Hochladen ihren Namen (`sound_map.h`). Neuer Web-Tab „Speicher“ mit allen Sounds der SD-Karte, Auswahlliste je Platz, Hochladen und Löschen. Neuer Lua-Ordner „Sound-Zuordnung“ (Felder 182–185).
- Vorhandene `loop.wav` … `sound24.wav` bleiben auf ihren Plätzen, die Zuordnung steht im Export/Backup.
- Noch nicht auf Hardware getestet.

## v7.23 (RC_Common 1.3.0)
- Fix: lange Lua-Auswahlfelder (Port-B-Baudrate) kommen vollständig an (Chunk-Fix).
- Ordner zeigen ihren Wert, z. B. „Motor (L 3)“. Felder ohne Wirkung werden ausgeblendet, Feldnummern unverändert.
- RC-System im Lua nur noch Anzeige (Umstellen in der Web-UI), sonst schaltete man sich per Lua das Lua selbst ab.

## v7.22
- Lua: Kanal-Felder (Motor, Sounds, Speed) bleiben bei Quelle „Aus“, „Dauer“ oder ohne Pin gesperrte Zahlenfelder statt Info „Deaktiviert“ (sonst erst nach Verlassen des Menüs änderbar).

## v7.21 (RC_Common 1.1.0)
- Gemeinsame Basis RC_Common (gleich in Multiswitch und Soundmodul Mini). Arduino-ESP32-Core 3.0.7 (PlatformIO über pioarduino) statt 2.0.17.
- CRSF-Bibliothek 1.1.0 mit Router: Port B mit Rolle CRSF reicht Frames zwischen Port A und Port B weiter (Modulkette).
- Port-B-Rollen mit gemeinsamen Nummern: Aus, CRSF, S.Port, GPS, Raw (Hott entfällt). Alter Wert wird umgerechnet. Lua-Feld 171 heißt jetzt „Port B Rolle“.
- Gemeinsame NVS-Schlüssel, Lua-Menü als Tabelle (`buildLuaMenu()`), Feld-IDs unverändert.
- Neu: SBUS-Zeitlimit 2 s (vorher nur Failsafe-Bit).
- Neu: GPS-Baudrate „Auto“ (sucht die Rate, stellt das GPS auf 115200 Baud / 10 Hz). Port C (SoftwareSerial) höchstens 38400.
- Web-API: `portb_role` statt `portb_mode` (alte Sicherungen werden umgerechnet).
- Entfernt: `port_function.*`, `lib/port_registry`, `test/test_native`, `test/test_crsf` (Tests jetzt in RC_Common).

## v7.20
- Port B kann zusätzlich die Rolle GPS übernehmen (Hardware-UART GPIO32/33). Steht Port B auf GPS, bleibt Port C aus.

## v7.19
- CRSF-Code als gemeinsame Bibliothek `src/CRSF_ESP32/` (v1.0.0, auch im Multiswitch).
- Fixes laut CRSF-Spezifikation: Empfangs-Parser verliert keine Frames mehr, Link-Statistik korrekt, Command-CRC (0xBA) korrigiert, FOLDER-Eintrag ohne überzähliges Byte.

## v7.18
- Fix: CRSF „Parameter Version Number“ im DEVICE_INFO war fest 0. Jetzt die Firmware-Version, damit TBS Agent geänderte Lua-Felder neu einliest.

## v7.17
- Fix: Lautstärkeänderung während einer laufenden Web-Testwiedergabe wirkt sofort.

## v7.16
- SD-Aktivitäts-Cooldown vor NVS-Schreibvorgängen (Brownout-Reset-Loop bei Modul ohne Stützkondensator). Alle Speicherpfade über den verzögerten Schreibvorgang in `loop()`.

## v7.15
- Kein sofortiger `enableAP()`-Aufruf mehr aus Web-/CRSF-Handlern.

## v7.14
- Neu: „WLAN dauerhaft an“ (`config.WifiAlwaysOn`, Vorgabe an, Lua-Feld 181).

## v7.11 – v7.13
- Rücknahme von v7.07–v7.10. Datei-Handles werden beim Upload freigegeben statt reserviert, Upload-Diagnose, Klartext-Fehlermeldungen.

## v7.07 – v7.10 (zurückgezogen)
- Anhebung von `max_files` zerstörte den WLAN-Start (siehe README, „Hinweis für Mitentwickler“).

## v7.06
- Sound-Upload abgesichert: Datei wird vor dem Ersetzen geschlossen und danach neu geladen.

## v7.04 / v7.05
- MKan auch bei SBUS-Betrieb, über zwei SBUS-Gruppenkanäle.

## v7.00
- Eigenständiges Projekt (Fork von v6.12). WLAN manuell schaltbar und Auto-Failsafe.
