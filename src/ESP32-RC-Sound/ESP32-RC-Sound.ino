/*
   ESP32-RC-Sound - Soundmodul-Firmware fuer RC-Modelle (ESP32)
   PiperPilot

   Eigenstaendiges Projekt. firmware-v5, firmware-v6 und firmware-v7 sind
   drei getrennte, unabhaengig baubare Projektstaende (kein gemeinsamer
   Code-Stand, keine gemeinsame Versionszaehlung).

   ── Basis ────────────────────────────────────────────────────────────────────
   Technische Basis ist Frank Verfuerths Projekt "ESP32-SBUS-Switch" (2022) -
   von dort stammt der grundlegende Ansatz fuer SBUS/CRSF/S.Port-Protokoll-
   implementierung, WLAN-Weboberflaeche zur Konfiguration und I2S-Audio, der
   seither allen hier weiterentwickelten Projekten zugrunde liegt. Das
   Soundmodul selbst (Hardware wie Firmware) ist daraus als eigenstaendiges,
   neu entworfenes Projekt hervorgegangen (Repository urspruenglich unter
   github.com/Ziege-One/ESP32-RC-Sound, seither unter dem Autorennamen
   PiperPilot fortgefuehrt).

   ── Ueberblick ────────────────────────────────────────────────────────────────
   Das Modul spielt WAV-Sounds (ein Motorsound-Zyklus start/loop/shut plus
   bis zu 24 frei zuordenbare Zusatzsounds) von einer SD-Karte ueber I2S
   ab, gesteuert ueber den RC-Bus (SBUS oder CRSF/ELRS) oder - bei V1/V2-
   Hardware zusaetzlich - ueber direkte PWM-/GPIO-Eingaenge. Konfiguriert
   wird es wahlweise per Weboberflaeche (WLAN) oder direkt am Sender ueber
   das native CRSF-/Lua-Parametermenue.

   ── Hardware-Varianten ───────────────────────────────────────────────────────────
   V1  GPIO-Pins {22, 0, 2, 4}   – BUS + PWM-Eingang + GPIO-Pin + Einkanal
   V2  GPIO-Pins {14,27,32,33}   – BUS + PWM-Eingang + GPIO-Pin + Einkanal
   V3  kein GPIO-Eingang          – nur BUS-Kanal + Einkanal (SBUS/CRSF)
                                     + Port B/C (siehe unten)
   Ein alter NVS-Stand mit den frueher existierenden Werten V4/V5 wird beim
   Booten auf V3 begrenzt, Port B/C starten dabei auf "Aus" (siehe
   config.cpp/loadConfig()).

   QUELLEN (V1/V2): BUS Kanal Low/High (1-16), PWM-Eingang Low/High (1-6),
                     GPIO-Pin direkt (1-6), Einkanal (1-8), MKan (1-16),
                     Dauerbetrieb, Deaktiviert
   QUELLEN (V3):     BUS Kanal Low/High (1-16), Einkanal (1-8), MKan (1-16),
                     Dauerbetrieb, Deaktiviert
   MOTOR SPEED:      V1/V2: BUS Kanal + PWM Pin  |  V3: BUS Kanal

   ── Pin-Belegung (alle Versionen) ──────────────────────────────────────────
   GPIO 13: WiFi-Aktivierung (LOW beim Booten = AP-Modus)
   GPIO 16: CRSF RX / SBUS RX  |  GPIO 17: CRSF TX
   GPIO 05: SD_CS  |  GPIO 18: SD_CLK  |  GPIO 19: SD_MISO  |  GPIO 23: SD_MOSI
   GPIO 21: I2S_DOUT  |  GPIO 25: I2S_LRC  |  GPIO 26: I2S_BCLK
   Port B (nur V3, zusätzlich): GPIO 32 RX  |  GPIO 33 TX
     Schaltung (S.Port-Rolle): GPIO33 -[1N4148 Anode->Kathode]-> S.Port Signal <- GPIO32

   ── Das Port-Modell (V3) ──────────────────────────────────────────────────
   Port A = RC-Eingang, fest (CRSF oder SBUS-Familie).
   Port B = echter zweiter Hardware-UART (GPIO32/33), Rolle waehlbar in Web
            UND CRSF/Lua-Menue, mit frei einstellbarer Baudrate je Rolle:
            Rollen aus RC_Common (uart_roles.h, gleiche Nummern in allen
            Modulen): Aus, CRSF (Modulkette: der CRSF-Router reicht Frames
            zwischen Port A und Port B weiter, nur bei RC-System CRSF),
            S.Port-Master (LiPo-Telemetrie), GPS oder Raw-UART.
            Port C hat keinen freien Hardware-UART (UART0 = USB/Debug,
            UART1/2 = Port A und B) und laeuft per SoftwareSerial. CRSF
            (420000 Baud) und S.Port sind darauf nicht zuverlaessig moeglich,
            deshalb bleibt Port C nur GPS an/aus.
   Port C = GPS (SoftwareSerial GPIO27/14) fuer CRSF-GPS-Telemetrie
            (Geschwindigkeit/Position an den Sender) - unabhaengiger
            An/Aus-Schalter. Kollidiert mit SBUS als Bus-Protokoll (beide
            belegen GPIO27), siehe setup()/gpsPinConflict.
   Start der Rollen, Telemetrie und Baudraten-Listen kommen aus RC_Common
   (uart_ports.h), gleich wie in Multiswitch und Soundmodul Mini.

   ── WLAN & Weboberflaeche ─────────────────────────────────────────────────
   Der Access Point (SoftAP) laesst sich auf drei Wegen aktivieren:
     - GPIO13 kurz auf GND beim Einschalten (immer verfuegbar, auch ganz
       ohne Sender).
     - Manueller Schalter in Web-Tab "WiFi" bzw. CRSF/Lua-Feld 174 ("WLAN
       jetzt") - wirkt sofort, ohne Neustart, ohne Persistierung.
     - Automatischer Failsafe (Web-Tab "WiFi"/Lua-Felder 175-176): schaltet
       das WLAN von selbst ein, wenn laenger als die eingestellte Zeit
       (5-240s, Standard 60s) kein gueltiges RC-Signal anliegt - schaltet
       NUR ein, nie automatisch wieder aus (verhindert, dass eine laufende
       WLAN-Konfiguration/ein laufendes OTA-Update abbricht, sobald kurz
       wieder ein gueltiges Signal hereinkommt).
   Ueber die Weboberflaeche lassen sich Sounds/Konfiguration verwalten,
   Sound-Dateien und Konfigurations-Backups hoch-/herunterladen bzw.
   loeschen, und eine neue Firmware direkt per WLAN aufspielen (OTA, siehe
   WebServerManager.cpp) - ohne das Modul auszubauen. Empfehlung aus der
   Praxis: ein OTA-Update moeglichst bei ueber ESC/Akku bestromtem Modul
   durchfuehren, nicht nur ueber USB (manche USB-Anschluesse liefern beim
   Flash-Schreiben nicht genug Strom, ein dadurch ausgeloester Brownout-
   Reset bricht den Upload ab). WiFi.softAP() gilt projektweit als die
   fehleranfaelligste Einzeloperation: ein Start mit noch stark belegtem/
   fragmentiertem Heap (z.B. nachdem Sounds/Uploads liefen) kann
   fehlschlagen - enableAP() (WebServerManager.cpp) loggt deshalb bei
   jedem Start den freien Heap und meldet einen Fehlschlag explizit.

   ── CRSF-/Lua-Parametermenue ───────────────────────────────────────────
   Bei RC-System ELRS (CRSF) ist das Modul zusaetzlich vollstaendig ueber
   das native CRSF-Parametermenue des Senders konfigurierbar (z.B. als
   Lua-Script in EdgeTX). 181 Feld-IDs, lueckenlos 1..181 (Tabelle in buildLuaMenu()) -
   die vollstaendige Feldliste steht direkt beim Parameter-System weiter
   unten. Jedes Modul erhaelt seine CRSF-Busadresse (0xC0+WM-Adresse) und
   seinen Telemetrie-Zeitslot aus der WM-Adresse (Wilhelm-Meier-Schema),
   damit sich mehrere Module am selben Bus nicht gegenseitig stoeren.

   ── Weitere Module dieses Projekts ───────────────────────────────────────
   config.cpp/.h         Persistente Konfiguration (NVS/Preferences),
                          Migration alter Datenformate.
   WebServerManager.cpp   Weboberflaeche, REST-API, OTA-Update.
   XT_I2S_Audio.cpp/.h    WAV-Wiedergabe ueber I2S, SD-Kartenzugriff.
   src/RC_Common/         Gemeinsame Basis aller ESP32-Module (identische
                          Kopie in jedem Projekt, siehe README dort):
                          CRSF-Bibliothek mit Router, UART-Rollen,
                          S.Port-LiPo, GPS, Lua-Menue-Tabelle, gemeinsame
                          NVS-Schluessel, WLAN-Failsafe, OTA-Update.

   Rahmenbedingung dieses Projekts: Aenderungen werden ohne Zugriff auf
   einen Compiler oder ein echtes Geraet vorgenommen - die Probe am
   Geraet obliegt dem Nutzer.
*/

#include <Arduino.h>
#include "XT_I2S_Audio.h"
#include <WiFi.h>
#include "sbus.h"
#include <SoftwareSerial.h>   // EspSoftwareSerial, Port C (GPS)
#include "src/RC_Common/rc_common.h"
#include "src/RC_Common/CRSF_ESP32/crsf_esp32.h"
#include "config.h"
#include "WebServerManager.h"
#include "src/RC_Common/uart_ports.h"     // Port B: Rollen starten, Telemetrie
#include "src/RC_Common/sport_lipo.h"
#include "src/RC_Common/gps_speed.h"
#include "src/RC_Common/lua_menu.h"       // CRSF-/Lua-Menue als Tabelle
#include "src/RC_Common/wifi_failsafe.h"

uint16_t Version = 721;  // Firmware-Version fuer Web-/Lua-Anzeige: major*100+minor (721 -> "7.21")
char versionString[6];
bool gpsPinConflict = false; // true, wenn Port C (GPS) UND SBUS gleichzeitig gewaehlt sind (GPIO27-Konflikt, siehe setup())
// Ob der AP gerade aktiv ist, wird NICHT hier gespiegelt: loop() fragt dafuer
// direkt WebServerManager::isApActive() ab (die eigentliche Variable lebt in
// WebServerManager.cpp) - WLAN kann jederzeit zur Laufzeit ein-/ausgeschaltet
// werden (manueller Schalter oder Auto-Failsafe, siehe wifiFailsafeCheck()),
// eine separate lokale Kopie des Zustands koennte sonst aus dem Takt geraten
// und server.handleClient() wuerde trotz aktivem AP nicht mehr aufgerufen.

// ── Zustand ───────────────────────────────────────────────────────────
bool Sound_on[RCSOUND_NUM_SLOTS]     = {};
bool Sound_play[RCSOUND_NUM_SLOTS]   = {};
bool Sound_on_web[RCSOUND_NUM_SLOTS] = {};

bool Sound_on_Motor       = false;
bool Sound_on_Motor_state = false;
bool engine_break         = false;
unsigned long shutdown_timer = 0;

enum Engine_State : uint8_t { OFF, STARTING, RUNNING, STOPPING };
volatile uint8_t engine_State = OFF;

// Boot-Snapshot: RC-System, Port-B-Rolle/Baudrate und Port-C(GPS)-Zustand
// werden einmalig in setup() aus config uebernommen. Ein spaeteres Aendern
// ueber Web/Lua (Felder 161/171-173) schreibt sofort in config, wirkt aber
// bewusst erst nach einem Neustart - die zugehoerigen UARTs sind schon
// initialisiert und duerfen waehrend des Betriebs nicht umgeschaltet werden.
int      Einkanal_RC_System_boot = 0;
uint8_t  PortB_Role_boot        = UROLE_OFF;
uint32_t PortB_Baud_boot        = 420000;
int      PortC_GPS_Enabled_boot = 0;
// Port B (GPIO32/33) als Zusatz-UART fuer RC_Common (uart_ports.h)
static UartPort portB = {"Port B", nullptr, 32, 33, UROLE_OFF, 0, UROLE_OFF, false, false};
// Port C (GPS): SoftwareSerial, RX=GPIO27 (GPS-TX), TX=GPIO14 (GPS-RX)
static SoftwareSerial gpsSoftSerial;
// Liefert den fuer Port B freien Hardware-UART - Port A belegt bereits den
// jeweils anderen (CRSF=Serial2, SBUS-Familie=Serial1).
static HardwareSerial* portBFreeSerial() {
    return (config.Einkanal_RC_System == 4) ? &Serial1 : &Serial2;
}
uint16_t einkanal_Data           = 0;   // 1 Bit/Schalter - Quelltyp "Einkanal" 70-77
uint16_t einkanal_SpeicherWM     = 0;
// Persistente Bit-Register je SBUS-Gruppe (analog zu einkanal_SpeicherWM,
// aber pro Gruppe getrennt) - siehe einkanalFunctionSBUSGroup().
uint16_t einkanal_SpeicherWM_Grp[RCSOUND_NUM_GROUPS] = {};

// Je ein 8-Bit-Wert (1 Bit/Schalter) aus den 2 unabhaengig konfigurierbaren
// Gruppen sowie deren Kombination zu einem 16-Bit-Flachwert - Quelltyp
// "MKan" 80-95 (siehe Config()). Je nach RC-System wird eine Gruppe entweder
// per CRSF-Gruppenadresse (config.EK_Gruppen_Adresse[], siehe
// updateGroupFromAddress()) oder per fest zugewiesenem SBUS-Kanal
// (config.SBUS_Gruppen_Channel[], siehe einkanalFunctionSBUSGroup())
// befuellt - beide Wege sind gegenseitig exklusiv, da loop() immer nur
// einen der beiden Busse gleichzeitig bedient.
uint8_t  einkanal_Gruppe[RCSOUND_NUM_GROUPS] = {};
uint32_t einkanal_Flat16          = 0;

int      Source_Ebenen_Um_Kanal_wert = 0;
int      Source_Ebenen_Kanal_wert    = 0;


// ── Hardware ──────────────────────────────────────────────────────────
#define SD_CS    5
#define I2S_DOUT 21
#define I2S_BCLK 26
#define I2S_LRC  25
const uint8_t WifiPin = 13;

// GPIO-Pins: Index 0-1 = CRSF RX/TX (immer), 2-5 = Eingangs-Pins
// V1: {16,17, 22, 0, 2, 4}
// V2: {16,17, 14,27,32,33}
// V3: nicht verwendet
uint8_t Input_Pin[6] = {16, 17, 22, 0, 2, 4};  // Default V1

// ── Throttle / Rampe ─────────────────────────────────────────────────
unsigned long previousTime_ramp = 0;
float last_throttle = 0.0f;
float throttle      = 0.0f;

char ssid[32];
char password[64];
unsigned long currentTime = 0;

// ── SBUS ──────────────────────────────────────────────────────────────
bfs::SbusRx   sbus_rx(&Serial1, 16, 27, true);
bfs::SbusData sbus_data;
bool BUS_OK = false;

// ── CRSF ──────────────────────────────────────────────────────────────
CRSF crsf;       // Port A - der Haupt-RC-Bus; mit Port B = CRSF zugleich Router der Modulkette
#define MWset       0x01
#define MWprop      0x02
#define MWset4      0x07
#define MWset4m     0x09
#define Multiswitch 0xA1

static constexpr unsigned long CRSF_TIMEOUT_MS = 2000;
static unsigned long lastCrsfPacket = 0;

static void checkCrsfTimeout() {
    if (Einkanal_RC_System_boot != 4) return;
    if (millis() - lastCrsfPacket > CRSF_TIMEOUT_MS) BUS_OK = false;
}

// NEU v7.21: SBUS-Zeitlimit. Bisher wurde nur das Failsafe-Bit im Frame
// ausgewertet; kommen gar keine Frames mehr (Kabel ab, Empfaenger ohne
// Strom), blieb BUS_OK auf true und Sounds liefen weiter. Gleiche Grenze
// wie bei CRSF.
static constexpr unsigned long SBUS_TIMEOUT_MS = CRSF_TIMEOUT_MS;
static unsigned long lastSbusFrameMs = 0;

static void checkSbusTimeout() {
    if (Einkanal_RC_System_boot == 4) return;
    if (BUS_OK && millis() - lastSbusFrameMs > SBUS_TIMEOUT_MS) BUS_OK = false;
}

uint16_t channel_output[16] = {};

// ── I2S Audio ─────────────────────────────────────────────────────────
XT_I2S_Class I2SAudio(I2S_LRC, I2S_BCLK, I2S_DOUT, I2S_NUM_0);

XT_Wav_Class Sound_loop ("/loop.wav");
XT_Wav_Class Sound_shut ("/shut.wav");
XT_Wav_Class Sound_start("/start.wav");
XT_Wav_Class Sound1("/sound1.wav");   XT_Wav_Class Sound2("/sound2.wav");
XT_Wav_Class Sound3("/sound3.wav");   XT_Wav_Class Sound4("/sound4.wav");
XT_Wav_Class Sound5("/sound5.wav");   XT_Wav_Class Sound6("/sound6.wav");
XT_Wav_Class Sound7("/sound7.wav");   XT_Wav_Class Sound8("/sound8.wav");
// v5: Slots 9-24 neu (Ebenen-Erweiterung) - gleiches Namensschema, /soundNN.wav auf SD
XT_Wav_Class Sound9 ("/sound9.wav");  XT_Wav_Class Sound10("/sound10.wav");
XT_Wav_Class Sound11("/sound11.wav"); XT_Wav_Class Sound12("/sound12.wav");
XT_Wav_Class Sound13("/sound13.wav"); XT_Wav_Class Sound14("/sound14.wav");
XT_Wav_Class Sound15("/sound15.wav"); XT_Wav_Class Sound16("/sound16.wav");
XT_Wav_Class Sound17("/sound17.wav"); XT_Wav_Class Sound18("/sound18.wav");
XT_Wav_Class Sound19("/sound19.wav"); XT_Wav_Class Sound20("/sound20.wav");
XT_Wav_Class Sound21("/sound21.wav"); XT_Wav_Class Sound22("/sound22.wav");
XT_Wav_Class Sound23("/sound23.wav"); XT_Wav_Class Sound24("/sound24.wav");

XT_Wav_Class* Sounds[RCSOUND_NUM_SLOTS] = {
    nullptr,
    &Sound1,&Sound2,&Sound3,&Sound4,&Sound5,&Sound6,&Sound7,&Sound8,
    &Sound9,&Sound10,&Sound11,&Sound12,&Sound13,&Sound14,&Sound15,&Sound16,
    &Sound17,&Sound18,&Sound19,&Sound20,&Sound21,&Sound22,&Sound23,&Sound24
};

// ── PWM-Eingang (V1/V2) ───────────────────────────────────────────────
volatile unsigned int PWM_pulse_width[6] = {3000,3000,3000,3000,3000,3000};
volatile unsigned int PWM_prev_time[6]   = {0,0,0,0,0,0};
// Zeitstempel des letzten gueltigen Pulses je Kanal - ohne das wuerden
// abgerissene/eingefrorene PWM-Signale ihren letzten Wert behalten und
// faelschlich weiter als gueltiger Zustand durchgehen (siehe pwmValid()).
volatile unsigned long PWM_lastPulseMs[6] = {0,0,0,0,0,0};
static constexpr unsigned long PWM_TIMEOUT_MS = 300;

static inline void IRAM_ATTR handlePWM(uint8_t idx) {
    if (digitalRead(Input_Pin[idx])) PWM_prev_time[idx] = micros();
    else { PWM_pulse_width[idx] = micros() - PWM_prev_time[idx]; PWM_lastPulseMs[idx] = millis(); }
}
void IRAM_ATTR ISR_PWM_0() { handlePWM(0); }
void IRAM_ATTR ISR_PWM_1() { handlePWM(1); }
void IRAM_ATTR ISR_PWM_2() { handlePWM(2); }
void IRAM_ATTR ISR_PWM_3() { handlePWM(3); }
void IRAM_ATTR ISR_PWM_4() { handlePWM(4); }
void IRAM_ATTR ISR_PWM_5() { handlePWM(5); }
void (*ISR_PWM[6])() = {ISR_PWM_0,ISR_PWM_1,ISR_PWM_2,ISR_PWM_3,ISR_PWM_4,ISR_PWM_5};

static bool pwm_isr_attached[6] = {};
void attachPWM_ISR(uint8_t pinIdx) {
    // Nur in V1/V2 aktiv
    if (config.Hardware_Config >= 2) return;
    if (pinIdx < 6 && !pwm_isr_attached[pinIdx]) {
        attachInterrupt(digitalPinToInterrupt(Input_Pin[pinIdx]), ISR_PWM[pinIdx], CHANGE);
        pwm_isr_attached[pinIdx] = true;
    }
}

// ── Vorwärtsdeklarationen ─────────────────────────────────────────────
void Config(); void SDCardInit(); void LoadFiles();
void AllocateSoundBuffers(); // reserviert die Sample-Puffer erst in setup() NACH dem WLAN-AP-Start, siehe dort und XT_I2S_Audio.h
static inline bool pwmValid(uint8_t idx);
void einkanalFunction(uint16_t ch);
void einkanalFunctionSBUSGroup(uint8_t g, uint16_t ch); // SBUS-Pendant zu updateGroupFromAddress() fuer "MKan" (Quelltyp 80-95)
void einkanalFunctionCRSF(); uint8_t compressSwitches(uint16_t state);
static void updateGroupFromAddress(uint8_t addr, uint8_t bits); // Gruppenadressen-Abgleich fuer "MKan" (Quelltyp 80-95)
float ramp_throttle(float t); float floatMap(float x,float a,float b,float c,float d);
void handleSound(uint8_t idx, XT_Wav_Class* snd);

// ======== CRSF-Parametersystem (181 Parameter) ==========================
// lib/sound_slots/sound_slots.h und test/test_sound_slots/ sind nicht Teil
// des esp32dev-Builds (nichts in src/ bindet sie ein) und bleiben ungenutzt
// liegen.
//
//  0: Root  1: Version Info
//  2: Folder Motor {3..15}
//   3: Sel MotorMode  4: Sel EINModus  5: Sel QuelleEIN  6: U8 KanalEIN
//   7: Sel QuelleSpd  8: U8 KanalSpd   9: U8 Vol
//  10: U8 Drzmin  11: U8 Drzmax/2  12: U8 Standgas  13: U8 Rampe  14: U8 Totband
//  15: Info HW-Version (aktuell aktive Hardware)
//
//  Sound s(1-24): fi=16+(s-1)*6  → endet bei 159
//   fi+0:Folder fi+1:Sel Quelle fi+2:U8 Kanal fi+3:U8 Vol fi+4:Sel Mode fi+5:Sel Test
//
//  160: Folder Einstellungen {161..169, 171..181}
//   161:Sel RC-Sys  162:U8 ModAdr  163:U8 EKKanal  164:Sel EKMode
//   165:Sel HardwareConfig  V1;V2;V3  (siehe config.h)
//   166:U8 PWM min(/16)  167:U8 PWM max(/16)
//   168:U8 Gruppenadr.1  169:U8 Gruppenadr.2  (255=aus, nur CRSF)
//  170: Info Gerätename
//   171:Sel Port B Rolle  Aus;CRSF;S.Port;GPS;Raw (RC_Common, uart_roles.h;
//       bis v7.20: Aus;CRSF;S.Port;Hott;Raw;GPS)
//   172:Sel Port B Baudrate  uartBaudOptions() (feste Liste, im Web frei)
//   173:Sel Port C (GPS)  Aus;An
//   174:Sel WLAN jetzt  Aus;Ein  (wirkt SOFORT, nicht persistiert - siehe luaSetWifiNow)
//   175:Sel Auto-WLAN bei Signalverlust  Aus;Ein  (persistiert, siehe config.WifiAutoEnable)
//   176:U8 Auto-WLAN Timeout (s)  (5-240, siehe config.WifiAutoTimeoutSec)
//   177:U8 SBUS Gruppe1 Kanal (255=aus)  178:Sel SBUS Gruppe1 Adresse  Adr0-3
//   179:U8 SBUS Gruppe2 Kanal (255=aus)  180:Sel SBUS Gruppe2 Adresse  Adr0-3
//   (177-180: SBUS-Pendant zu 168/169, siehe EK_Gruppen_Adresse/
//   SBUS_Gruppen_Channel in config.h)
//   181:Sel WLAN dauerhaft an  Aus;Ein  (persistiert, siehe config.WifiAlwaysOn;
//   Default AN - ueberstimmt 174/175/176 komplett, siehe wifiFailsafeCheck())


// ── CRSF-Geraeteadresse + Ping-Slot aus der WM-Adresse (Wilhelm-Meier-Schema) ──
// Jedes Modul bekommt eine eindeutige CRSF-Bus-Adresse aus 0xC0..0xCF.
// Adresse   = 0xC0 + WM-Adresse         (WM 0 -> 0xC0, WM 2 -> 0xC2, ...)
// Slot-Nr   = (Adresse - 0xC0) * 2 = WM-Adresse * 2   (Wilhelm Meiers Formel)
// Die Ping-Antwort/Telemetrie wird erst im eigenen Zeit-Slot gesendet, damit
// sich die Antworten mehrerer Module am selben Empfaenger nicht ueberlappen.
#define CRSF_SLOT_MS 5   // Dauer pro Slot-Einheit (ms): Soundmodul (WM2=Slot4) antwortet 20ms versetzt zum Multiswitch (WM0=Slot0)
static inline uint8_t  crsfAddrFromWM()  { return 0xC0 + (uint8_t)constrain(config.modul_adress, 0, 15); }
static inline uint16_t crsfSlotDelayMs() { return (uint16_t)((uint8_t)constrain(config.modul_adress, 0, 15) * 2) * CRSF_SLOT_MS; }

// ── Typ-Mapping Quelle Motor/Sound EIN ───────────────────────────────
// Typ: 0=BUS-L(0-15) 1=BUS-H(20-35) 2=PWM-L(40-45) 3=PWM-H(50-55)
//      4=Pin(60-65)  5=Einkanal(70-77) 6=Mehrfachkanal/MKan(80-95,
//      siehe RCSOUND_NUM_GROUPS/EK_Gruppen_Adresse - 2 Gruppen x 8 Kanaele)
//      7=Dauerbetrieb(200) 8=Deaktiviert(999)
// V3 ignoriert Typen 2/3/4 in Config()

static uint8_t einTyp(int c) {
    if(c==999)return 8; if(c==200)return 7;
    if(c>=80&&c<=95)return 6; if(c>=70&&c<=77)return 5;
    if(c>=60&&c<=65)return 4; if(c>=50&&c<=55)return 3;
    if(c>=40&&c<=45)return 2; if(c>=20&&c<=35)return 1;
    if(c>=0&&c<=15)return 0; return 8;
}

// Prueft, ob ein "Quelle"-Rohwert einem der von einTyp() erkannten,
// gueltigen Bereiche entspricht. Bewusst NICHT static: config.cpp (Boot-
// Migration in loadConfig()) und WebServerManager.cpp (Eingabepruefung in
// handleApiSound(), greift auch bei jedem Konfigurations-Import) nutzen
// dieselbe Definition wie einTyp() - keine zweite, potenziell abweichende
// Bereichspruefung. Ein ungueltiger Rohwert (z.B. aus einer alten
// Konfigurationsdatei mit einem inzwischen entfallenen Wertebereich) wird
// beim Laden/Import auf 999 (Deaktiviert) zurueckgesetzt, statt unveraendert
// in config.Source_Start_Sound[] stehen zu bleiben.
bool isValidEinSource(int c) {
    return c==999 || c==200 ||
           (c>=80&&c<=95) || (c>=70&&c<=77) || (c>=60&&c<=65) ||
           (c>=50&&c<=55) || (c>=40&&c<=45) || (c>=20&&c<=35) ||
           (c>=0&&c<=15);
}
static uint8_t einNr(int c) {
    if(c>=80&&c<=95)return c-80+1; if(c>=70&&c<=77)return c-70+1;
    if(c>=60&&c<=65)return c-60+1; if(c>=50&&c<=55)return c-50+1;
    if(c>=40&&c<=45)return c-40+1; if(c>=20&&c<=35)return c-20+1;
    if(c>=0&&c<=15)return c+1; return 1;
}
static int setEin(uint8_t t, uint8_t nr) {
    uint8_t i=(nr>0)?nr-1:0;
    if(t==0)return min(i,(uint8_t)15);
    if(t==1)return 20+min(i,(uint8_t)15);
    if(t==2)return 40+min(i,(uint8_t)5);
    if(t==3)return 50+min(i,(uint8_t)5);
    if(t==4)return 60+min(i,(uint8_t)5);
    if(t==5)return 70+min(i,(uint8_t)7);
    if(t==6)return 80+min(i,(uint8_t)15);
    if(t==7)return 200; return 999;
}
static uint8_t einMaxNr(uint8_t t) {
    if(t==0||t==1)return 16; if(t==2||t==3||t==4)return 6;
    if(t==5)return 8; if(t==6)return 16; return 1;
}

// ── Typ-Mapping Quelle Speed ──────────────────────────────────────────
// Typ: 0=BUS Kanal(0-15) 1=PWM Pin(20-25) 2=Deaktiviert(999)
// V3 ignoriert Typ 1 in Config()
static uint8_t spdTyp(int c) {
    if(c>=0&&c<=15)return 0; if(c>=20&&c<=25)return 1; return 2;
}
static uint8_t spdNr(int c) {
    if(c>=0&&c<=15)return c+1; if(c>=20&&c<=25)return c-20+1; return 1;
}
static int setSpd(uint8_t t, uint8_t nr) {
    uint8_t i=(nr>0)?nr-1:0;
    if(t==0)return min(i,(uint8_t)15);
    if(t==1)return 20+min(i,(uint8_t)5);
    return 999;
}

static bool testSoundActive[RCSOUND_NUM_SLOTS]={};

// ── Hilfsfunktion: Aktuelle HW-Version als String ─────────────────────
static const char* hwVersionStr() {
    switch(config.Hardware_Config) {
        case 0: return "V1 (GPIO 22,0,2,4)";
        case 1: return "V2 (GPIO 14,27,32,33)";
        default: return "V3 (Port A/B/C)"; // S.Port/GPS sind Port-B/C-Rollen, keine eigene Hardware-Stufe
    }
}

// ── CRSF Quelle-Speed Optionen je nach HW ─────────────────────────────
static const char* spdSelStr() {
    return (config.Hardware_Config < 2)
        ? "BUS Kanal;PWM Pin;Deaktiviert"
        : "BUS Kanal;Deaktiviert";
}
static uint8_t spdSelMax() {
    return (config.Hardware_Config < 2) ? 2 : 1;
}
static uint8_t spdSelVal() {
    uint8_t t = spdTyp(config.Source_Speed_Sound_0);
    // V3: PWM (t==1) → zeige als Deaktiviert
    if (config.Hardware_Config >= 2 && t == 1) return 1;
    return t;
}

// ── CRSF Quelle EIN Optionen: V3 zeigt nur L/H/EK/MKan/Dauer/Aus ────────
// "MKan" (Mehrfachkanal, Typ 6) ist fuer alle Hardware-Varianten waehlbar,
// auch V3 (kein GPIO) - siehe Config() fuer die Auswertung. Genau die
// Hardware, mit der die Multiswitch-Box/das WM-Widget normalerweise
// betrieben wird.
static const char* einSelStr() {
    return (config.Hardware_Config < 2)
        ? "L;H;PWM-L;PWM-H;Pin;EK;MKan;Dauer;Aus"
        : "L;H;EK;MKan;Dauer;Aus";
}
static uint8_t einSelMax() { return (config.Hardware_Config < 2) ? 8 : 5; }
static uint8_t einSelVal(int cfg) {
    uint8_t t = einTyp(cfg);
    if (config.Hardware_Config >= 2) {
        // V3+: PWM/Pin → Deaktiviert (5, kein GPIO auf dieser Hardware)
        if (t == 2 || t == 3 || t == 4) return 5;
        // V3+ Mapping: 0=L,1=H,5→2=EK,6→3=Eben,7→4=Dauer,8→5=Aus
        if (t==5) return 2; if (t==6) return 3; if (t==7) return 4; if (t>=8) return 5;
        return t; // 0,1
    }
    return t; // V1/V2: vollständig 0-8
}
static int einSelWrite(uint8_t val, int oldCfg) {
    uint8_t nr = einNr(oldCfg);
    if (config.Hardware_Config >= 2) {
        // V3+ Mapping Auswahl→Typ: 0=L,1=H,2=EK,3=Eben,4=Dauer,5=Aus
        static const uint8_t v3map[] = {0,1,5,6,7,8};
        uint8_t t = (val < 6) ? v3map[val] : 8;
        return setEin(t, nr);
    }
    return setEin(val, nr); // V1/V2: direkt
}

// ======== CRSF-/Lua-Parametermenue als Tabelle (RC_Common LuaMenu) =====
// Seit v7.21 steht jedes Feld genau einmal in buildLuaMenu() (Feld-ID,
// Ordner, Typ, Name, Grenzen, Lese-/Schreibfunktion); Senden und Schreiben
// laufen generisch ueber src/RC_Common/lua_menu.h. Die Feld-IDs sind
// unveraendert (Liste oben), gespeicherte Sender-Profile laufen weiter.
// Felder, die je nach Zustand anders aussehen (z. B. "Kanal Nr" wird bei
// "Dauer"/"Aus" zur Info), regeln das ueber eine view-Funktion.
static LuaItem luaItems[182];
static LuaMenu luaMenu(luaItems, sizeof(luaItems) / sizeof(luaItems[0]));

static void luaShowVersion(uint8_t, LuaView& v, char* buf) {
    snprintf(buf, LUA_VIEW_BUF, "%s %s", versionString, hwVersionStr());
    v.text = buf;
}
static void luaShowHardware(uint8_t, LuaView& v, char* buf) {
    if (hardwareConfigRestartPending()) {
        snprintf(buf, LUA_VIEW_BUF, "%s (Neustart noetig!)", hwVersionStr());
        v.name = "Hardware (aktiv)";
        v.text = buf;
    } else {
        v.text = hwVersionStr();
    }
}
static void luaShowDeviceName(uint8_t, LuaView& v, char* buf) {
    snprintf(buf, LUA_VIEW_BUF, "%s (Web aendern)", config.Device_Name);
    v.text = buf;
}
static void luaShowSoundFolder(uint8_t s, LuaView& v, char* buf) {
    snprintf(buf, LUA_VIEW_BUF, "Sound %d", s);
    v.name = buf;
}

// ── Motor ────────────────────────────────────────────────────────────────
static uint8_t luaGetThrottleMode(uint8_t)          { return (uint8_t)constrain(config.throttle_mode, 0, 1); }
static void    luaSetThrottleMode(uint8_t, uint8_t v){ config.throttle_mode = v; markDirty(); }
static uint8_t luaGetToggle(uint8_t)                { return (uint8_t)constrain(config.engine_on_toggle, 0, 1); }
static void    luaSetToggle(uint8_t, uint8_t v)     { config.engine_on_toggle = v; markDirty(); }
static uint8_t luaGetMinSpeed(uint8_t)              { return (uint8_t)constrain(config.Min_Speed_Sound_0, 0, 200); }
static void    luaSetMinSpeed(uint8_t, uint8_t v)   { config.Min_Speed_Sound_0 = v; markDirty(); }
static uint8_t luaGetMaxSpeed(uint8_t)              { return (uint8_t)constrain(config.Max_Speed_Sound_0 / 2, 50, 255); }
static void    luaSetMaxSpeed(uint8_t, uint8_t v)   { config.Max_Speed_Sound_0 = constrain((int)v * 2, 100, 510); markDirty(); }
static uint8_t luaGetShutdown(uint8_t)              { return (uint8_t)constrain(config.shutdowndelay, 0, 60); }
static void    luaSetShutdown(uint8_t, uint8_t v)   { config.shutdowndelay = v; markDirty(); }
static uint8_t luaGetRamp(uint8_t)                  { return (uint8_t)constrain(config.throttle_ramp, 0, 50); }
static void    luaSetRamp(uint8_t, uint8_t v)       { config.throttle_ramp = v; markDirty(); }
static uint8_t luaGetDeadBand(uint8_t)              { return (uint8_t)constrain(config.throttle_dead_band, 0, 50); }
static void    luaSetDeadBand(uint8_t, uint8_t v)   { config.throttle_dead_band = v; markDirty(); }

// Quelle Speed: V3 0=BUS,1=Deakt  V1/V2: 0=BUS,1=PWM,2=Deakt
static void    luaShowSpdSrc(uint8_t, LuaView& v, char*) { v.text = spdSelStr(); v.max = spdSelMax(); }
static uint8_t luaGetSpdSrc(uint8_t)                { return spdSelVal(); }
static void    luaSetSpdSrc(uint8_t, uint8_t v) {
    uint8_t nr = spdNr(config.Source_Speed_Sound_0);
    uint8_t realTyp = (config.Hardware_Config >= 2 && v == 1) ? 2 : v;
    config.Source_Speed_Sound_0 = setSpd(realTyp, nr); markDirty();
}
static void luaShowSpdChannel(uint8_t, LuaView& v, char*) {
    uint8_t t = spdTyp(config.Source_Speed_Sound_0);
    if (t == 2 || (config.Hardware_Config >= 2 && t == 1)) {
        v.type = LUA_INFO; v.name = "Kanal Speed"; v.text = "Deaktiviert";
    } else {
        v.max = (t == 1) ? 6 : 16;
    }
}
static uint8_t luaGetSpdChannel(uint8_t)            { return spdNr(config.Source_Speed_Sound_0); }
static void    luaSetSpdChannel(uint8_t, uint8_t v) { config.Source_Speed_Sound_0 = setSpd(spdTyp(config.Source_Speed_Sound_0), v); markDirty(); }

// ── Motor (arg 0) und Sound 1-24 (arg = Sound-Nr.) ───────────────────────
static void    luaShowSource(uint8_t, LuaView& v, char*) { v.text = einSelStr(); v.max = einSelMax(); }
static uint8_t luaGetSource(uint8_t s)              { return einSelVal(config.Source_Start_Sound[s]); }
static void    luaSetSource(uint8_t s, uint8_t v)   { config.Source_Start_Sound[s] = einSelWrite(v, config.Source_Start_Sound[s]); markDirty(); }
static void luaShowChannel(uint8_t s, LuaView& v, char*) {
    uint8_t t = einTyp(config.Source_Start_Sound[s]);
    bool noPin = (config.Hardware_Config >= 2 && (t == 2 || t == 3 || t == 4));
    if (t == 7 || t == 8 || noPin) {
        v.type = LUA_INFO;
        if (s == 0) { v.name = "Kanal EIN"; v.text = (t == 7 || noPin) ? "Dauerbetrieb/Deakt." : "Deaktiviert"; }
        else        { v.text = (t == 7) ? "Dauerbetrieb" : "Deaktiviert"; }
    } else {
        v.max = einMaxNr(t);
    }
}
static uint8_t luaGetChannel(uint8_t s)             { return einNr(config.Source_Start_Sound[s]); }
static void    luaSetChannel(uint8_t s, uint8_t v)  { config.Source_Start_Sound[s] = setEin(einTyp(config.Source_Start_Sound[s]), v); markDirty(); }
static uint8_t luaGetVolume(uint8_t s)              { return (uint8_t)constrain(config.Volumen_Sound[s], 0, 200); }
static void    luaSetVolume(uint8_t s, uint8_t v)   { config.Volumen_Sound[s] = v; markDirty(); }
static uint8_t luaGetPlayMode(uint8_t s)            { return (uint8_t)constrain(config.Mode_Sound[s], 0, 2); }
static void    luaSetPlayMode(uint8_t s, uint8_t v) { config.Mode_Sound[s] = v; markDirty(); }
// "Test Sound" wirkt sofort und wird nicht gespeichert
static uint8_t luaGetTest(uint8_t s)                { return testSoundActive[s] ? 1 : 0; }
static void    luaSetTest(uint8_t s, uint8_t v)     { if (v == 1) Sound_on_web[s] = true; testSoundActive[s] = (v == 1); }

// ── Einstellungen ────────────────────────────────────────────────────────
static uint8_t luaGetRcSystem(uint8_t)              { return (uint8_t)constrain(config.Einkanal_RC_System, 0, 4); }
static void    luaSetRcSystem(uint8_t, uint8_t v)   { config.Einkanal_RC_System = v; markDirty(); }
static uint8_t luaGetModAdr(uint8_t)                { return (uint8_t)constrain(config.modul_adress, 0, 20); }
static void    luaSetModAdr(uint8_t, uint8_t v)     { config.modul_adress = v; crsf.setDeviceAddress(crsfAddrFromWM()); markDirty(); }
// Kanal mit 999 = aus: arg 0 Einkanal, 1/2 SBUS-Gruppe 1/2
static int*    luaChannelRef(uint8_t a)             { return a == 0 ? &config.Einkanal_Channel : &config.SBUS_Gruppen_Channel[a - 1]; }
static uint8_t luaGetBusChannel(uint8_t a)          { int c = *luaChannelRef(a); return c == 999 ? 255 : (uint8_t)constrain(c, 0, 15); }
static void    luaSetBusChannel(uint8_t a, uint8_t v){ *luaChannelRef(a) = (v == 255) ? 999 : constrain(v, 0, 15); markDirty(); }
// Modus Normal (0) oder WM0-WM3 (10-13): arg 0 Einkanal, 1/2 SBUS-Gruppe 1/2
static int*    luaModeRef(uint8_t a)                { return a == 0 ? &config.Einkanal_mode : &config.SBUS_Gruppen_Mode[a - 1]; }
static uint8_t luaGetWmMode(uint8_t a)              { int m = *luaModeRef(a); return (m >= 10 && m <= 13) ? (uint8_t)(m - 9) : 0; }
static void    luaSetWmMode(uint8_t a, uint8_t v)   { *luaModeRef(a) = (v == 0) ? 0 : constrain((int)v + 9, 10, 13); markDirty(); }
// Hardware Config wirkt erst nach einem Neustart (siehe loadConfig())
static uint8_t luaGetHwConfig(uint8_t)              { return (uint8_t)constrain(config.Hardware_Config_Pending, 0, 2); }
static void    luaSetHwConfig(uint8_t, uint8_t v) {
    config.Hardware_Config_Pending = v; markDirty();
    Serial.printf("!!! Hardware Config -> %d (wirkt erst nach Neustart) – NEUSTART ERFORDERLICH !!!\n", v);
}
static uint8_t luaGetPwm(uint8_t a)                 { return (uint8_t)(constrain(a ? config.PWM_scale_max : config.PWM_scale_min, 0, 4080) / 16); }
static void    luaSetPwm(uint8_t a, uint8_t v)      { (a ? config.PWM_scale_max : config.PWM_scale_min) = constrain((int)v * 16, 0, 4080); markDirty(); }
static uint8_t luaGetGroupAdr(uint8_t a)            { return (uint8_t)constrain(config.EK_Gruppen_Adresse[a], 0, 255); }
static void    luaSetGroupAdr(uint8_t a, uint8_t v) { config.EK_Gruppen_Adresse[a] = v; markDirty(); }
// Port B/C wirken erst nach einem Neustart (Boot-Snapshot in setup())
static uint8_t luaGetPortBRole(uint8_t)             { return uartRoleClamp(config.PortB_Role); }
static void    luaSetPortBRole(uint8_t, uint8_t v) {
    if (v != config.PortB_Role) {
        // Baudrate auf den Standard der Rolle vorbelegen (Aus/Raw: unveraendert)
        uint32_t def = uartRoleDefaultBaud(v);
        if (def != 0 && v != UROLE_RAW) config.PortB_Baud = def;
    }
    config.PortB_Role = v; markDirty();
    Serial.printf("!!! Port B Rolle -> %s (wirkt erst nach Neustart) – NEUSTART ERFORDERLICH !!!\n", uartRoleName(v));
}
static uint8_t luaGetPortBBaud(uint8_t)             { return uartBaudToIndex(config.PortB_Baud); }
static void    luaSetPortBBaud(uint8_t, uint8_t v) {
    config.PortB_Baud = uartBaudFromIndex(v); markDirty();
    Serial.printf("!!! Port B Baudrate -> %u (wirkt erst nach Neustart) – NEUSTART ERFORDERLICH !!!\n", (unsigned)config.PortB_Baud);
}
static uint8_t luaGetPortCGps(uint8_t)              { return config.PortC_GPS_Enabled ? 1 : 0; }
static void    luaSetPortCGps(uint8_t, uint8_t v) {
    config.PortC_GPS_Enabled = v; markDirty();
    Serial.printf("!!! Port C (GPS) -> %d (wirkt erst nach Neustart) – NEUSTART ERFORDERLICH !!!\n", v);
}
// 174 "WLAN jetzt" wirkt sofort und wird nicht gespeichert; der naechste
// Start haengt wieder von GPIO13 bzw. "WLAN dauerhaft an" ab.
static uint8_t luaGetWifiNow(uint8_t)               { return WebServerManager::isApActive() ? 1 : 0; }
static void    luaSetWifiNow(uint8_t, uint8_t v)    { if (v) WebServerManager::enableAP(); else WebServerManager::disableAP(); }
static uint8_t luaGetWifiAuto(uint8_t)              { return config.WifiAutoEnable ? 1 : 0; }
static void    luaSetWifiAuto(uint8_t, uint8_t v)   { config.WifiAutoEnable = v; markDirty(); }
static uint8_t luaGetWifiTimeout(uint8_t)           { return (uint8_t)constrain(config.WifiAutoTimeoutSec, 5, 240); }
static void    luaSetWifiTimeout(uint8_t, uint8_t v){ config.WifiAutoTimeoutSec = v; markDirty(); }
// 181 "WLAN dauerhaft an" wirkt erst ab dem naechsten Start (kein
// enableAP() aus dem CRSF-Handler heraus, siehe Kommentar in setup())
static uint8_t luaGetWifiAlways(uint8_t)            { return config.WifiAlwaysOn ? 1 : 0; }
static void    luaSetWifiAlways(uint8_t, uint8_t v) { config.WifiAlwaysOn = v; markDirty(); }

static void buildLuaMenu() {
    static const char* AUS_EIN  = "Aus;Ein";
    static const char* WM_MODES = "Normal;WM0;WM1;WM2;WM3";
    LuaMenu& m = luaMenu;
    m.clear();
    m.folder(0, 0, "");
    m.info(1, 0, "Version", "")->view = luaShowVersion;

    m.folder(2, 0, "Motor");
    m.select(3, 2, "Motor Mode", "Eine Richtung;Zwei Richtungen", luaGetThrottleMode, luaSetThrottleMode);
    m.select(4, 2, "Motor EIN Modus", "Normal;Toggle", luaGetToggle, luaSetToggle);
    m.select(5, 2, "Quelle Motor", einSelStr(), luaGetSource, luaSetSource, 0)->view = luaShowSource;
    m.u8(6, 2, "Kanal Nr EIN", 1, 16, "", luaGetChannel, luaSetChannel, 0)->view = luaShowChannel;
    m.select(7, 2, "Quelle Speed", spdSelStr(), luaGetSpdSrc, luaSetSpdSrc)->view = luaShowSpdSrc;
    m.u8(8, 2, "Kanal Nr Speed", 1, 16, "", luaGetSpdChannel, luaSetSpdChannel)->view = luaShowSpdChannel;
    m.u8(9, 2, "Volumen Motor", 0, 200, "", luaGetVolume, luaSetVolume, 0);
    m.u8(10, 2, "Drehzahl min %", 0, 200, "%", luaGetMinSpeed, luaSetMinSpeed);
    m.u8(11, 2, "Drehzahl max /2", 50, 255, "%", luaGetMaxSpeed, luaSetMaxSpeed);
    m.u8(12, 2, "Motor aus Standgas", 0, 60, "s", luaGetShutdown, luaSetShutdown);
    m.u8(13, 2, "Motor Rampe %/s", 0, 50, "", luaGetRamp, luaSetRamp);
    m.u8(14, 2, "Standgas Totband", 0, 50, "%", luaGetDeadBand, luaSetDeadBand);
    m.info(15, 2, "Hardware", "")->view = luaShowHardware;

    // Sound s (1-24): Ordner fi = 16 + (s-1)*6, Felder fi+1 .. fi+5
    for (uint8_t s = 1; s <= 24; s++) {
        uint8_t fi = 16 + (s - 1) * 6;
        LuaItem* f = m.folder(fi, 0, "Sound");
        f->arg = s; f->view = luaShowSoundFolder;
        m.select(fi + 1, fi, "Quelle", einSelStr(), luaGetSource, luaSetSource, s)->view = luaShowSource;
        m.u8(fi + 2, fi, "Kanal Nr", 1, 16, "", luaGetChannel, luaSetChannel, s)->view = luaShowChannel;
        m.u8(fi + 3, fi, "Volumen", 0, 200, "", luaGetVolume, luaSetVolume, s);
        m.select(fi + 4, fi, "Wiedergabe Mode", "Normal;Loop;Tippbetrieb", luaGetPlayMode, luaSetPlayMode, s);
        m.select(fi + 5, fi, "Test Sound", AUS_EIN, luaGetTest, luaSetTest, s);
    }

    m.folder(160, 0, "Einstellungen");
    m.select(161, 160, "RC-System", "FrSky;FlySky;ELRS SBUS;Hott;ELRS CRSF", luaGetRcSystem, luaSetRcSystem);
    m.u8(162, 160, "Modul Adresse", 0, 20, "", luaGetModAdr, luaSetModAdr);
    m.u8(163, 160, "Einkanal Kanal (255=aus)", 0, 255, "", luaGetBusChannel, luaSetBusChannel, 0);
    m.select(164, 160, "Einkanal Mode", WM_MODES, luaGetWmMode, luaSetWmMode, 0);
    m.select(165, 160, "Hardware Config", "V1;V2;V3", luaGetHwConfig, luaSetHwConfig);
    m.u8(166, 160, "PWM min (/16 us)", 0, 255, "", luaGetPwm, luaSetPwm, 0);
    m.u8(167, 160, "PWM max (/16 us)", 0, 255, "", luaGetPwm, luaSetPwm, 1);
    m.u8(168, 160, "Gruppenadr. 1 (255=aus)", 0, 255, "", luaGetGroupAdr, luaSetGroupAdr, 0);
    m.u8(169, 160, "Gruppenadr. 2 (255=aus)", 0, 255, "", luaGetGroupAdr, luaSetGroupAdr, 1);
    m.info(170, 0, "Geraetename", "")->view = luaShowDeviceName;
    m.select(171, 160, "Port B Rolle", uartRoleOptions(), luaGetPortBRole, luaSetPortBRole);
    m.select(172, 160, "Port B Baudrate", uartBaudOptions(), luaGetPortBBaud, luaSetPortBBaud);
    m.select(173, 160, "Port C (GPS)", "Aus;An", luaGetPortCGps, luaSetPortCGps);
    m.select(174, 160, "WLAN jetzt", AUS_EIN, luaGetWifiNow, luaSetWifiNow);
    m.select(175, 160, "Auto-WLAN bei Signalverlust", AUS_EIN, luaGetWifiAuto, luaSetWifiAuto);
    m.u8(176, 160, "Auto-WLAN Timeout (s)", 5, 240, "", luaGetWifiTimeout, luaSetWifiTimeout);
    m.u8(177, 160, "SBUS Gruppe1 Kanal (255=aus)", 0, 255, "", luaGetBusChannel, luaSetBusChannel, 1);
    m.select(178, 160, "SBUS Gruppe1 Modus", WM_MODES, luaGetWmMode, luaSetWmMode, 1);
    m.u8(179, 160, "SBUS Gruppe2 Kanal (255=aus)", 0, 255, "", luaGetBusChannel, luaSetBusChannel, 2);
    m.select(180, 160, "SBUS Gruppe2 Modus", WM_MODES, luaGetWmMode, luaSetWmMode, 2);
    m.select(181, 160, "WLAN dauerhaft an", AUS_EIN, luaGetWifiAlways, luaSetWifiAlways);
}

void setup() {
    Serial.begin(115200);
    loadConfig();

    strncpy(ssid,    config.WiFi_SSID,    sizeof(ssid)-1);    ssid[sizeof(ssid)-1]='\0';
    strncpy(password,config.WiFi_Password,sizeof(password)-1); password[sizeof(password)-1]='\0';

    // CRSF oder SBUS starten
    if (config.Einkanal_RC_System == 4) {
        crsf.init_crsf(&Serial2, 16, 17);
        crsf.setDeviceAddress(crsfAddrFromWM());   // eindeutige CRSF-Adresse aus WM-Adresse
        Serial.printf("CRSF (RX=16, TX=17)  Geraeteadresse 0x%02X  Slot %d\n",
                      crsfAddrFromWM(), (uint8_t)constrain(config.modul_adress,0,15) * 2);
    } else {
        sbus_rx.Begin();
        Serial.println("SBUS gestartet");
    }

    // GPIO-Pins je nach Hardware-Version
    switch (config.Hardware_Config) {
        case 0: { uint8_t p[]={16,17,22, 0, 2, 4}; memcpy(Input_Pin,p,6); break; }
        case 1: { uint8_t p[]={16,17,14,27,32,33}; memcpy(Input_Pin,p,6); break; }
        default: break;  // V3/V4: Input_Pin nicht verwendet
    }

    pinMode(WifiPin, INPUT_PULLUP);

    // INPUT_PULLUP für Eingangs-Pins (V1/V2 only, GPIO16/17 bei CRSF überspringen)
    if (config.Hardware_Config < 2) {
        for (uint8_t i = 2; i < 6; i++) {
            if (config.Einkanal_RC_System == 4 &&
                (Input_Pin[i] == 16 || Input_Pin[i] == 17)) continue;
            pinMode(Input_Pin[i], INPUT_PULLUP);
        }
    }
    // CRSF RX/TX-Pins nie als INPUT_PULLUP (gilt für alle Versionen)
    // GPIO16/17 werden durch crsf.init_crsf() / sbus_rx.Begin() konfiguriert

    SDCardInit();
    LoadFiles();

    // begin() registriert nur die HTTP-Routen und merkt sich SSID/Passwort -
    // das passiert IMMER, unabhaengig vom WifiPin-Zustand beim Booten, weil
    // WLAN auch spaeter zur Laufzeit (manueller Schalter oder Auto-Failsafe)
    // gestartet werden koennen muss. Das eigentliche Einschalten von AP-Funk
    // + Webserver-Socket passiert davon getrennt, nur bedingt, ueber
    // enableAP() (siehe WebServerManager.h/.cpp).
    WebServerManager::begin(ssid, password);

    // "WLAN dauerhaft an" (config.WifiAlwaysOn, Default AN, siehe config.h)
    // schaltet den AP unconditional ein und ueberstimmt damit den
    // GPIO13-Bootpin komplett - wer den Pin-Bootmodus weiterhin will, muss
    // die Option zuerst explizit ausschalten (Web-Tab "WiFi" oder CRSF/Lua-
    // Feld 181).
    if (config.WifiAlwaysOn) {
        Serial.println("WLAN dauerhaft aktiv (config.WifiAlwaysOn) - schalte AP ein...");
        WebServerManager::enableAP();
    } else if (!digitalRead(WifiPin)) {
        Serial.println("AP Modus (WifiPin beim Booten auf LOW)...");
        WebServerManager::enableAP();
    }

    // Der SD-Hintergrund-Lese-Task startet bewusst erst HIER, NACH dem
    // WLAN-AP-Start (siehe XT_I2S_Audio.cpp) - nicht schon im Konstruktor
    // des globalen I2SAudio-Objekts, also nicht vor setup()/WiFi-Start.
    I2SAudio.StartSdReaderTask();

    // Reserviert die Sample-Puffer aller Sound-Objekte (siehe
    // AllocateSoundBuffers()/XT_I2S_Audio.h) ebenfalls erst HIER, NACH dem
    // WLAN-AP-Start, statt sie als feste .bss-Arrays seit Programmstart
    // mitzuschleppen - WiFi.softAP() laeuft dadurch auf einem moeglichst
    // freien/unfragmentierten Heap.
    AllocateSoundBuffers();

    Einkanal_RC_System_boot = config.Einkanal_RC_System;
    PortB_Role_boot         = config.PortB_Role;
    PortB_Baud_boot         = config.PortB_Baud;
    PortC_GPS_Enabled_boot  = config.PortC_GPS_Enabled;
    sprintf(versionString, "%d.%02d", Version/100, Version%100);
    saveConfig();

    // Port B (GPIO32/33, echter 2. Hardware-UART): Rolle starten ueber
    // RC_Common (uart_ports.h). CRSF = Router der Modulkette (nur wenn
    // Port A CRSF ist), S.Port = LiPo-Sensoren, GPS, Raw = UART offen.
    // portBFreeSerial() ist immer der UART, den Port A nicht belegt.
    sportLipoUsePollIds(config.sport_poll_id);
    portB.ser  = portBFreeSerial();
    portB.role = PortB_Role_boot;
    portB.baud = PortB_Baud_boot;
    uartPortsBegin(&portB, 1, crsf, Einkanal_RC_System_boot == 4);

    // Port C (GPS) - unabhaengiger An/Aus-Schalter. Kollidiert mit SBUS als
    // Bus-Protokoll (beide belegen GPIO27), siehe gpsPinConflict.
    if (PortC_GPS_Enabled_boot && portB.activeRole == UROLE_GPS) {
        // Es gibt nur ein GPS - Port B (Hardware-UART) hat Vorrang.
        Serial.println("Port C (GPS) bleibt aus: GPS laeuft bereits auf Port B.");
    } else if (PortC_GPS_Enabled_boot) {
        if (config.Einkanal_RC_System != 4) {
            gpsPinConflict = true;
            Serial.println("!!! WARNUNG: Port C (GPS) zusammen mit SBUS gewaehlt !!!");
            Serial.println("!!! GPIO27 wird von SBUS UND GPS gleichzeitig belegt -> GPS bleibt deaktiviert !!!");
            Serial.println("!!! Fuer GPS-Speed bitte Einkanal/RC-System auf CRSF (4) stellen. !!!");
        } else {
            gpsSoftSerial.begin(9600, SWSERIAL_8N1, 27, 14);
            gpsSpeedInitStream(&gpsSoftSerial);
            Serial.println("Port C: GPS (SoftwareSerial RX=27 TX=14, 9600 Baud)");
        }
    }

    buildLuaMenu();
    Serial.printf("RC_Common %s, Lua-Menue %u Felder\n", RC_COMMON_VERSION, (unsigned)luaMenu.size());
    Serial.printf("ESP32-RC-Sound %s – Hardware: %s\n", versionString, hwVersionStr());
}

// Auto-WLAN-Failsafe, angelehnt an ELRS-Empfaenger: schaltet das WLAN
// automatisch ein, wenn eine Zeit lang kein gueltiges RC-Signal anliegt -
// z.B. wenn das Modell auf der Werkbank liegt und der Sender aus ist. Nutzt
// den bereits vorhandenen, fuer CRSF UND SBUS gemeinsam gepflegten BUS_OK-
// Flag (siehe checkCrsfTimeout()/sbus_data.failsafe) statt einer zweiten,
// eigenen Signalverlust-Erkennung. Schaltet NUR EIN, nie automatisch wieder
// aus: einmal aktiviertes WLAN bleibt an bis zum manuellen Ausschalten
// (Web/Lua) oder Neustart - ein automatisches Wieder-Abschalten wuerde eine
// laufende Konfiguration/ein laufendes OTA-Update abwuergen, sobald kurz
// wieder ein gueltiges RC-Signal hereinkaeme. config.WifiAutoEnable/
// WifiAutoTimeoutSec siehe config.h/.cpp.
// Ein WLAN-Start zur Laufzeit (statt wie beim Booten VOR
// AllocateSoundBuffers()) laeuft mit potenziell staerker fragmentiertem
// Heap - historisch die fehleranfaelligste Operation in dieser Codebase.
// enableAP() loggt deshalb bei jedem Aufruf den freien Heap vor dem Start.
//
// "WLAN dauerhaft an" (config.WifiAlwaysOn, siehe config.h/setup()) schaltet
// den AP bereits beim Booten fest ein - diese Funktion greift dann gar nicht
// mehr ein, siehe Abbruch ganz oben.
static WifiFailsafe wifiFailsafe;   // RC_Common, gleiche Logik in allen Modulen

void wifiFailsafeCheck() {
    if (wifiFailsafe.check(millis(), config.WifiAlwaysOn, config.WifiAutoEnable,
                           config.WifiAutoTimeoutSec, BUS_OK, WebServerManager::isApActive())) {
        Serial.printf("Auto-WLAN-Failsafe: %u s ohne gueltiges RC-Signal - schalte WLAN ein.\n",
                      (unsigned)config.WifiAutoTimeoutSec);
        WebServerManager::enableAP();
    }
}

// ======== Loop =======================================================
void loop() {
    currentTime = millis();
    if (WebServerManager::isApActive()) WebServerManager::Webpage();

    // Port B (S.Port/GPS) und Port C (GPS) einlesen (RC_Common, ohne aktive
    // Rolle ohne Wirkung)
    uartPortsUpdate();

    if (Einkanal_RC_System_boot == 4) {
        crsf.read_packets(0);
        crsf.route_packets();   // Modulkette Port A <-> Port B (nur bei Rolle CRSF aktiv)
        for (int i=0;i<16;i++) channel_output[i]=crsf.get_crfs_channels(i);
        // BUS_OK ist daran gekoppelt, ob gerade ein gueltiger CRSF-Frame
        // geparst wurde (validFramesRx-Zaehler erhoeht) - nicht an
        // channel_output[0]>0, denn ein Kanalwert allein ist kein
        // zuverlaessiger Nachweis fuer einen frischen Frame (z.B. bei
        // legitim niedrigem Kanal-1-Wert).
        static uint32_t lastValidFrameCount = 0;
        uint32_t vf = crsf.getValidFrames();
        if (vf != lastValidFrameCount) { lastValidFrameCount = vf; BUS_OK = true; lastCrsfPacket = millis(); }
        checkCrsfTimeout();

        // Alle 100 ms Zellspannungen (S.Port) und GPS als CRSF-Telemetrie.
        // KEIN Batterie-Frame (0x08): RxBt bleibt die vom Empfaenger selbst
        // gemessene Spannung. Das Modul erscheint im TBS Agent trotzdem, weil
        // es auf jeden Ping mit device_info antwortet.
        uartPortsTelemetry(crsf);
        if(crsf.getDeviceInfoReplyPending() && (millis() - crsf.getPingTime() >= crsfSlotDelayMs())){
            // Erst im eigenen Zeit-Slot auf den Broadcast-Ping antworten (Wilhelm-Meier-Schema),
            // damit sich die Device-Info-Antworten mehrerer Module nicht ueberlappen.
            crsf.setDeviceInfoReplyPending(false);
            char dn[24]; snprintf(dn,sizeof(dn),"%s@%d",config.Device_Name,config.modul_adress);
            // NEU v7.18: Firmware-Version als CRSF "Parameter Version Number"
            // mitgeben (war zuvor fest 0x00, siehe Kommentar in
            // crsf_esp32.cpp/send_device_info()) - damit erkennen
            // Konfiguratoren wie TBS Agent eine geaenderte Parameterstruktur
            // zuverlaessig und lesen sie neu ein, statt eine veraltete
            // zwischengespeicherte Menuestruktur weiter anzuzeigen.
            crsf.send_device_info(dn,luaMenu.maxId(),(uint8_t)(Version & 0xFF));
        }
        if(crsf.getDeviceReadReplyPending()){
            uint8_t pi=crsf.getParamReadIndex();
            crsf.setDeviceReadReplyPending(false);
            luaMenu.send(crsf,pi);
        }
        if(crsf.getDeviceWriteReplyPending()){
            uint8_t wi=crsf.getParamWriteIndex(),wv=crsf.getParamWriteValue();
            crsf.setDeviceWriteReplyPending(false);
            luaMenu.write(wi,wv); luaMenu.send(crsf,wi);
        }
        if(crsf.getDeviceCommandReplyPending()){
            crsf.setDeviceCommandReplyPending(false);
            einkanalFunctionCRSF();
        }
    } else {
        if(sbus_rx.Read()){
            sbus_data=sbus_rx.data(); BUS_OK=true; lastSbusFrameMs=millis();
            if(!sbus_data.failsafe){
                if(config.Einkanal_Channel<16) einkanalFunction(sbus_data.ch[config.Einkanal_Channel]);
                // "MKan"-Gruppen ueber SBUS (siehe einkanalFunctionSBUSGroup()) -
                // je Gruppe ein eigener, fest zugewiesener Kanal (999 = aus).
                for(uint8_t g=0; g<RCSOUND_NUM_GROUPS; g++){
                    int c = config.SBUS_Gruppen_Channel[g];
                    if(c>=0 && c<16) einkanalFunctionSBUSGroup(g, sbus_data.ch[c]);
                }
            } else BUS_OK=false;
            for(int i=0;i<16;i++) channel_output[i]=sbus_data.ch[i];
        }
        checkSbusTimeout();
    }

    // NVS-Speicher: 500 ms nach der letzten Aenderung schreiben. Kurz genug, dass
    // eine Aenderung ein Ausschalten praktisch immer "ueberlebt" (frueher 2000 ms ->
    // Aenderung ging bei schnellem Ausschalten verloren), aber lang genug, dass
    // schnelle Schieberegler-Bursts weiterhin zu einem einzigen Schreibvorgang
    // gebuendelt werden (der Timer wird bei jeder Aenderung zurueckgesetzt).
    // v7.16: zusaetzlich zurueckgehalten, solange seit einer SD-Schreib-
    // aktivitaet (Sound-Upload/-Loeschen) noch der Cooldown laeuft (siehe
    // WebServerManager::sdActivityCooldownActive()) - Feldbefund: ohne
    // Stuetzkondensator am Modul kann ein sofortiger blockierender NVS-
    // Schreibvorgang direkt nach einem SD-Schreibvorgang die Versorgung so
    // weit einbrechen lassen, dass ein Brownout-Reset-Loop entsteht. Sowohl
    // die Web-Speichern-Buttons (handleApiConfigPost()/"/save"-Route in
    // WebServerManager.cpp) als auch das CRSF/Lua-Menue markieren nur noch
    // "dirty" statt sofort zu schreiben - dieser Check hier ist seitdem die
    // EINZIGE Stelle, an der saveConfigForce() waehrend des Betriebs
    // tatsaechlich aufgerufen wird (Ausnahme: saveConfig() einmalig am Ende
    // von setup()).
    if(configDirty&&(millis()-configDirtyMs)>=500UL&&!WebServerManager::sdActivityCooldownActive())
        saveConfigForce();

    // Auto-WLAN-Failsafe (siehe wifiFailsafeCheck()) - BUS_OK ist an dieser
    // Stelle im loop() fuer diesen Durchlauf bereits aktuell (CRSF-Zweig
    // setzt/prueft es oben ueber checkCrsfTimeout(), SBUS-Zweig direkt beim
    // Read()).
    wifiFailsafeCheck();

    Config();
    if(currentTime<5000) return;
    for(uint8_t i=1;i<=24;i++) handleSound(i,Sounds[i]);

    // Motor Toggle
    if(Sound_on[0]&&!Sound_on_Motor_state&&config.engine_on_toggle){Sound_on_Motor=!Sound_on_Motor;Sound_on_Motor_state=true;}
    else if(!Sound_on[0]&&Sound_on_Motor_state&&config.engine_on_toggle) Sound_on_Motor_state=false;
    else if(!config.engine_on_toggle) Sound_on_Motor=Sound_on[0];

    bool mw=Sound_on_Motor||Sound_on_web[0];
    if((mw&&!engine_break&&!Sound_play[0])||(throttle>0&&engine_break&&!Sound_play[0])||(throttle>0&&Sound_play[0])){Sound_play[0]=true;shutdown_timer=millis();}
    if(!mw){Sound_play[0]=false;engine_break=false;}
    if(config.shutdowndelay>0&&Sound_play[0]&&(millis()-shutdown_timer)>((unsigned long)config.shutdowndelay*1000UL)){Sound_play[0]=false;engine_break=true;}

    switch(engine_State){
        case OFF:    if(Sound_play[0]){Sound_start.Volume=config.Volumen_Sound[0];I2SAudio.Play(&Sound_start);engine_State=STARTING;} break;
        case STARTING: if(!Sound_start.Playing){throttle=0;last_throttle=0;Sound_loop.RepeatForever=true;Sound_loop.Volume=config.Volumen_Sound[0];I2SAudio.Play(&Sound_loop);engine_State=RUNNING;} break;
        case RUNNING:  Sound_loop.Volume=config.Volumen_Sound[0]; if(!Sound_play[0]){throttle=0;if(last_throttle==0){Sound_loop.RepeatForever=false;I2SAudio.Stop(&Sound_loop);engine_State=STOPPING;}} break;
        case STOPPING: if(!Sound_loop.Playing){Sound_shut.Volume=config.Volumen_Sound[0];I2SAudio.Play(&Sound_shut);engine_State=OFF;} break;
    }
    throttle=ramp_throttle(throttle);
    if(config.Min_Speed_Sound_0!=config.Max_Speed_Sound_0)
        Sound_loop.Speed=floatMap(throttle,0,100,config.Min_Speed_Sound_0,config.Max_Speed_Sound_0)/100.0f;
}

// ======== Sound ======================================================
void handleSound(uint8_t idx, XT_Wav_Class* snd) {
    if(!snd)return;
    if(Sound_on[idx]||Sound_on_web[idx]){
        snd->Volume=config.Volumen_Sound[idx];
        if(!Sound_play[idx]){Sound_play[idx]=true;snd->LoadWavFile();if(config.Mode_Sound[idx]==1)snd->RepeatForever=true;I2SAudio.Play(snd);Sound_on_web[idx]=false;}
    }else{
        snd->RepeatForever=false;
        // NEU v7.17: Lautstaerke auch hier weiter aktualisieren, solange der
        // Sound noch nachspielt (Sound_play[idx] true) - z.B. beim Web-
        // Testbutton (testSound()), der Sound_on_web[idx] bereits im selben
        // Frame wieder zuruecksetzt, in dem die Wiedergabe gestartet wird
        // (siehe oben). Ohne dies wirkte eine waehrend einer laufenden
        // Testwiedergabe im Web-UI geaenderte und gespeicherte Lautstaerke
        // erst beim naechsten Trigger, nicht auf die bereits laufende
        // Wiedergabe (Feldbefund, siehe README-Changelog v7.17). Der Motor-
        // Sound (idx 0, eigene Zustandsmaschine im loop()) war davon nie
        // betroffen - dort wird die Lautstaerke ohnehin bei jedem
        // loop()-Durchlauf im Zustand RUNNING neu gesetzt.
        if(Sound_play[idx]) snd->Volume=config.Volumen_Sound[idx];
        if(config.Mode_Sound[idx]==2&&Sound_play[idx])I2SAudio.Stop(snd);
        // testSoundActive[idx] (Anzeige "Test Sound" im CRSF-/Lua-Menue,
        // luaSetTest()) wird hier zurueckgesetzt, sobald das Ende
        // der Wiedergabe erkannt wird - der Web-Testbutton zeigt ohnehin
        // keinen dauerhaften Zustand. Gilt nicht fuer Loop-Mode
        // (RepeatForever): dort spielt der Sound
        // bewusst endlos weiter, "Ein" bleibt in dem Fall zurecht stehen.
        if(!snd->Playing&&Sound_play[idx]){snd->UnLoadWavFile();Sound_play[idx]=false;testSoundActive[idx]=false;}
    }
}

// ======== Config: Quellen auswerten ==================================
void Config() {
    bool isV3 = (config.Hardware_Config >= 2);  // V3 hat keine GPIO-Eingaenge (einziger Wert >=2)

    for (int x=0; x<=24; x++) {
        int src = config.Source_Start_Sound[x];
        if      (src>=0   && src<=15)  Sound_on[x] = BUS_OK && (channel_output[src] < 624);
        else if (src>=20  && src<=35)  Sound_on[x] = BUS_OK && (channel_output[src-20] > 1424);
        else if (src>=40  && src<=45)  {
            // PWM Low – nur V1/V2
            if (!isV3) { uint8_t pi=src-40; attachPWM_ISR(pi); unsigned int d=PWM_pulse_width[pi]; Sound_on[x]=pwmValid(pi)&&(d<2600)&&(map(d,config.PWM_scale_min,config.PWM_scale_max,0,100)<25); }
            else Sound_on[x]=false;
        }
        else if (src>=50  && src<=55)  {
            // PWM High – nur V1/V2
            if (!isV3) { uint8_t pi=src-50; attachPWM_ISR(pi); unsigned int d=PWM_pulse_width[pi]; Sound_on[x]=pwmValid(pi)&&(d<2600)&&(map(d,config.PWM_scale_min,config.PWM_scale_max,0,100)>75); }
            else Sound_on[x]=false;
        }
        else if (src>=60  && src<=65)  {
            // GPIO Pin – nur V1/V2
            if (!isV3) { uint8_t pi=src-60; Sound_on[x]=(pi<6)&&!digitalRead(Input_Pin[pi]); }
            else Sound_on[x]=false;
        }
        else if (src>=70  && src<=77)  Sound_on[x] = BUS_OK && bitRead(einkanal_Data, src-70);
        else if (src>=80  && src<=95) {
            // "MKan": 16 unabhaengige Einzelkanaele aus 2 Gruppen (je 8
            // Schalter, 1 Bit/Schalter = an/aus). Bei CRSF kommen die
            // Gruppen aus 2 frei waehlbaren Busadressen (siehe
            // einkanalFunctionCRSF()/updateGroupFromAddress()), bei SBUS
            // aus 2 fest zugewiesenen Kanaelen (siehe
            // einkanalFunctionSBUSGroup()) - beide fuellen gemeinsam
            // einkanal_Flat16. Jeder der 16 Werte ist ein eigener, direkt
            // gelesener Kanal.
            // nr 0..15  ->  Gruppe = nr/8 (0..1), Bit = nr%8 (0..7)
            uint8_t nr = (uint8_t)(src - 80);
            Sound_on[x] = BUS_OK && bitRead(einkanal_Flat16, nr);
        }
        else if (src==200)             Sound_on[x] = true;
        else                           Sound_on[x] = false;
    }

    // Motor Speed
    int spd = config.Source_Speed_Sound_0;
    if      (spd>=0 && spd<=15)        throttle = map(channel_output[spd], 82, 1900, 0, 100);
    else if (!isV3 && spd>=20 && spd<=25) { uint8_t pi=spd-20; attachPWM_ISR(pi); throttle = pwmValid(pi) ? map((long)PWM_pulse_width[pi], config.PWM_scale_min, config.PWM_scale_max, 0, 100) : 0; }
    else                               throttle = 0;
    throttle = constrain(throttle, 0, 100);

    if (config.throttle_mode) {
        if      (throttle > 50+config.throttle_dead_band) throttle=map((long)throttle,50+config.throttle_dead_band,100,0,100);
        else if (throttle < 50-config.throttle_dead_band) throttle=map((long)throttle,50-config.throttle_dead_band,0,0,100);
        else throttle=0;
    } else {
        throttle=(throttle>config.throttle_dead_band)?map((long)throttle,config.throttle_dead_band,100,0,100):0;
    }
    throttle = constrain(throttle, 0, 100);
}

// ======== Hilfsfunktionen ============================================
float floatMap(float x,float a,float b,float c,float d){if(b==a)return c;return(x-a)*(d-c)/(b-a)+c;}
// true nur, wenn der Kanal innerhalb PWM_TIMEOUT_MS einen frischen Puls
// gesehen hat - verhindert, dass ein abgerissenes/eingefrorenes PWM-Signal
// mit seinem letzten Wert weiter als "gueltig" durchgeht.
static inline bool pwmValid(uint8_t idx) {
    if (idx >= 6) return false;
    noInterrupts(); unsigned long last = PWM_lastPulseMs[idx]; interrupts();
    return (last != 0) && (millis() - last < PWM_TIMEOUT_MS);
}
// WICHTIG - max_files NICHT erhoehen (Erfahrung aus v7.07..v7.10):
// Hier stand voruebergehend SD.begin(SD_CS, SPI, 4000000, "/sd", 15), um das
// vfs_fat-Limit gleichzeitig offener Dateien (Standardwert max_files=5)
// anzuheben - wegen "vfs_fat: open: no free file descriptors" beim Upload.
// Das hat reproduzierbar das WLAN zerstoert: max_files=15 belegt beim Mounten
// dauerhaft einen deutlich groesseren Block im internen DRAM (je Datei ein
// FIL-Objekt inkl. 512-Byte-Sektorpuffer). Nicht die Kilobyte an sich sind
// das Problem, sondern dass dieser Block fest im Pool sitzt, BEVOR der
// WLAN-Treiber seine mehreren ZUSAMMENHAENGENDEN Puffer anfordert - daher
// "Expected to init 4 rx buffer, actual is 3" bzw. "rc_enable_trc fail,
// no mem" trotz >100 KB freiem Heap (Fragmentierung, kein Platzmangel).
// Belegt durch 7 von 7 Versionen: max_files=5 -> WLAN stabil (v7.03/05/06/11),
// max_files=15 -> WLAN defekt (v7.07/08/09/10).
// Der Deskriptor-Engpass wird stattdessen geloest, indem waehrend eines
// Uploads/Loeschens nicht benoetigte Handles FREIGEGEBEN werden - siehe
// releaseIdleMotorHandles() in WebServerManager.cpp.
void SDCardInit(){pinMode(SD_CS,OUTPUT);digitalWrite(SD_CS,HIGH);if(!SD.begin(SD_CS))Serial.println("FEHLER: SD!");}
void LoadFiles(){Sound_loop.LoadWavFile();Sound_shut.LoadWavFile();Sound_start.LoadWavFile();}

// Holt die Sample-Puffer ALLER Sound-Objekte vom Heap (siehe
// AllocateBuffers()/Kommentar in XT_I2S_Audio.h) - aufgerufen aus setup()
// NACH dem WLAN-AP-Start, damit WiFi.softAP() zuerst auf einem moeglichst
// freien/unfragmentierten Heap laeuft. Gefahrlos: LoadWavFile()/
// OpenWavFile() (vor dem WLAN-Start aufgerufen) lesen nur den WAV-Header in
// lokale Stack-Puffer, nicht in Samples/BackSamples - abgespielt
// (I2SAudio.Play()) wird ohnehin erst spaeter aus loop() heraus.
void AllocateSoundBuffers(){
    Sound_loop.AllocateBuffers();
    Sound_shut.AllocateBuffers();
    Sound_start.AllocateBuffers();
    for (int i = 1; i < RCSOUND_NUM_SLOTS; i++) {
        if (Sounds[i]) Sounds[i]->AllocateBuffers();
    }
}

float ramp_throttle(float tn){
    unsigned long rt=currentTime-previousTime_ramp; previousTime_ramp=currentTime;
    if(rt>1000)rt=1000; float step=(config.throttle_ramp/1000.0f)*(float)rt;
    if(tn>last_throttle){float s=last_throttle+step;tn=(s<tn)?s:tn;}
    else{float s=last_throttle-step;tn=(s>tn)?s:tn;}
    last_throttle=tn; return tn;
}

// ======== Einkanal SBUS ==============================================
// Decodiert die per Kanalwert "amplituden-codierte" WM-Adresse/Schalter/
// Status-Kombination (2 Bit Adresse, 3 Bit Schalter-Nr, 1 Bit Status) aus
// einem rohen SBUS-Kanalwert - Grundlage sowohl fuer den normalen Einkanal-
// WM-Modus (Einkanal_mode 10-13) unten als auch fuer die SBUS-Gruppen in
// einkanalFunctionSBUSGroup().
static void decodeSbusWm(uint16_t ch, uint8_t &addr, uint8_t &sw, uint8_t &st) {
    uint16_t n=0;uint8_t v=0;
    switch(config.Einkanal_RC_System){
        case 0:n=(ch>=172)?(ch-172+1):0;v=(uint8_t)(n>>4);break;
        case 1:n=(ch>=220)?(ch-220):0;v=(uint8_t)((n+(n>>6))>>4);break;
        case 2:n=(ch>=172)?(ch-172):0;v=(uint8_t)(n>>4);break;
        case 3:n=(ch>=205)?(ch-205):0;v=(uint8_t)(n>>4);break;
    }
    addr=(v>>4)&0b11; sw=(v>>1)&0b111; st=v&0b1;
}
void einkanalFunction(uint16_t ch) {
    einkanal_Data=ch;
    if(config.Einkanal_mode==0){einkanal_Data/=8;}
    else if(config.Einkanal_mode<=9){
        if(config.Einkanal_RC_System==0)einkanal_Data/=8;
        else if(config.Einkanal_RC_System==1){einkanal_Data=constrain(einkanal_Data,206,1837);einkanal_Data=((einkanal_Data-206)*10+20)/64;}
        else if(config.Einkanal_RC_System==2){float v=((float)einkanal_Data-172.0f+1.5f)*0.155677655677655f;einkanal_Data=(uint16_t)v;}
    }else{
        uint8_t addr,sw,st; decodeSbusWm(ch,addr,sw,st);
        if(addr==(uint8_t)(config.Einkanal_mode-10))bitWrite(einkanal_SpeicherWM,sw,st);
        einkanal_Data=einkanal_SpeicherWM;
    }
}

// ======== Einkanal-Gruppen SBUS (MKan-Aequivalent) =====================
// SBUS kennt - anders als CRSF - keine adressierten Pakete, daher braucht
// jede der beiden "MKan"-Gruppen hier einen eigenen, fest zugewiesenen
// SBUS-Kanal (config.SBUS_Gruppen_Channel[g], siehe loop()) statt einer
// frei waehlbaren Busadresse. config.SBUS_Gruppen_Mode[g] uebernimmt dabei
// dieselbe Werte-/Bedeutungskonvention wie config.Einkanal_mode beim
// einzelnen Einkanal-Kanal (siehe einkanalFunction() oben):
//   0      = "Normal"  - Kanalwert direkt (ch/8) als vollstaendiges 8-Bit-
//            Bitmuster fuer diese Gruppe uebernehmen, kein WM-Protokoll auf
//            diesem Kanal. Entspricht funktional dem "bits"-Wert, den
//            einkanalFunctionCRSF()/updateGroupFromAddress() aus einem
//            einzelnen CRSF-WM-Paket bekommt (dort liegt das volle 8-Bit-
//            Muster ja bereits fertig im Paket vor).
//   10-13  = "WM Adr 0..3" - derselbe amplituden-codierte WM-Modus wie beim
//            einzelnen Einkanal-Kanal (siehe decodeSbusWm() oben), da SBUS
//            hier (anders als "Normal") mehrere Schalter zeitversetzt ueber
//            denselben Kanal uebertraegt und Bit-fuer-Bit akkumuliert werden
//            muss.
// Das Ergebnis landet wie bei einkanalFunctionCRSF()/updateGroupFromAddress()
// in einkanal_Gruppe[g] und wird zu einkanal_Flat16 kombiniert - die
// "MKan"-Einzelkanaele (80-95) in Config() lesen CRSF- wie SBUS-Gruppen ueber
// denselben Flat16-Wert.
void einkanalFunctionSBUSGroup(uint8_t g, uint16_t ch) {
    if(config.SBUS_Gruppen_Mode[g]==0){
        einkanal_Gruppe[g]=(uint8_t)(ch/8);
    }else{
        uint8_t addr,sw,st; decodeSbusWm(ch,addr,sw,st);
        if(addr==(uint8_t)(config.SBUS_Gruppen_Mode[g]-10)) bitWrite(einkanal_SpeicherWM_Grp[g],sw,st);
        einkanal_Gruppe[g]=(uint8_t)einkanal_SpeicherWM_Grp[g];
    }
    einkanal_Flat16 = (uint32_t)einkanal_Gruppe[0] | ((uint32_t)einkanal_Gruppe[1] << 8);
}

// ======== Einkanal CRSF ==============================================
// Zusaetzlich zum Einkanal-Abgleich gegen die eigene Busadresse
// (config.modul_adress, Quelltyp 70-77) wird jede eingehende Schalter-
// Adresse auch gegen die 2 frei konfigurierbaren Gruppenadressen
// (config.EK_Gruppen_Adresse[]) geprueft. Trifft eine davon zu, landet der
// jeweilige 8-Bit-Wert (1 Bit/
// Schalter) in einkanal_Gruppe[g] und wird zu einkanal_Flat16 kombiniert -
// die Grundlage fuer die 16 "MKan"-Einzelkanaele (80-95) in Config().
static void updateGroupFromAddress(uint8_t addr, uint8_t bits){
    for(uint8_t g=0; g<RCSOUND_NUM_GROUPS; g++){
        if(config.EK_Gruppen_Adresse[g]!=255 && addr==(uint8_t)config.EK_Gruppen_Adresse[g]){
            einkanal_Gruppe[g]=bits;
        }
    }
    // Bewusst per Hand aufgezaehlt (2 Gruppen zu einem 16-Bit-Flachwert)
    // statt generisch ueber RCSOUND_NUM_GROUPS geloopt, damit ein Blick auf
    // diese Zeile sofort zeigt, wie viele Gruppen kombiniert werden - eine
    // kuenftige Aenderung von RCSOUND_NUM_GROUPS MUSS diese Zeile von Hand
    // mitziehen (siehe Kommentar bei RCSOUND_NUM_GROUPS in config.h).
    einkanal_Flat16 = (uint32_t)einkanal_Gruppe[0]
                     | ((uint32_t)einkanal_Gruppe[1] << 8);
}
void einkanalFunctionCRSF(){
    uint8_t wmc=crsf.get_cmd_buffer(5),cmd=crsf.get_cmd_buffer(6);
    if(wmc!=Multiswitch)return;
    switch(cmd){
        // MWset4: 8 Schalter, 2 Bit/Schalter (4 Zustaende) an einer Adresse.
        // einkanal_Data (Quelltyp 70-77) nutzt nur 1 Bit/Schalter aus
        // compressSwitches() und bleibt auf config.modul_adress bezogen.
        // Zusaetzlich (unabhaengig von modul_adress) wird dieselbe 1-Bit-
        // Auswertung an updateGroupFromAddress() gemeldet, damit sie auch
        // als eine der 2 Gruppen fuer "MKan" (80-95) zaehlen kann, falls die
        // Adresse dort konfiguriert ist.
        case MWset4:{
            uint8_t a=crsf.get_cmd_buffer(7);
            uint16_t s=((uint16_t)crsf.get_cmd_buffer(8)<<8)|crsf.get_cmd_buffer(9);
            uint8_t bits=compressSwitches(s);
            if(a==(uint8_t)config.modul_adress) einkanal_Data=bits;
            updateGroupFromAddress(a,bits);
            break;
        }
        case MWset4m:{
            uint8_t c=min((uint8_t)crsf.get_cmd_buffer(7),(uint8_t)7);
            for(uint8_t i=0;i<c;i++){
                uint8_t a=crsf.get_cmd_buffer(8+(3*i));
                uint16_t s=((uint16_t)crsf.get_cmd_buffer(9+(3*i))<<8)|crsf.get_cmd_buffer(10+(3*i));
                uint8_t bits=compressSwitches(s);
                if(a==(uint8_t)config.modul_adress) einkanal_Data=bits;
                updateGroupFromAddress(a,bits);
            }
            break;
        }
        // MWset liefert bereits 1 Bit/Schalter direkt (kein Kompaktieren noetig).
        case MWset:{
            uint8_t a=crsf.get_cmd_buffer(7);
            uint8_t b=crsf.get_cmd_buffer(8);
            if(a==(uint8_t)config.modul_adress) einkanal_Data=b;
            updateGroupFromAddress(a,b);
            break;
        }
        case MWprop:break;
    }
}
uint8_t compressSwitches(uint16_t s){uint8_t r=0;for(uint8_t i=0;i<8;i++)r|=((s>>(i*2))&0x1)<<i;return r;}

