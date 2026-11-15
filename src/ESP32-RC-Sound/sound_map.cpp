/*
 * sound_map.cpp  -  ESP32-RC-Sound, Sound-Zuordnung, siehe sound_map.h
 */

#include "sound_map.h"
#include "XT_I2S_Audio.h"
#include <SD.h>
#include <dirent.h>
#include <sys/stat.h>

// Sound-Objekte und Audio-Ausgabe aus ESP32-RC-Sound.ino
extern XT_Wav_Class  Sound_loop, Sound_shut, Sound_start;
extern XT_Wav_Class* Sounds[RCSOUND_NUM_SLOTS];
extern XT_I2S_Class  I2SAudio;
extern volatile uint8_t engine_State;

void soundMapDefaultName(int slot, char* out) {
    static const char* const MOTOR[3] = { "loop.wav", "shut.wav", "start.wav" };
    if (slot >= 0 && slot < 3) snprintf(out, SND_FILE_LEN, "%s", MOTOR[slot]);
    else if (slot >= 3 && slot < SND_FILE_SLOTS) snprintf(out, SND_FILE_LEN, "sound%d.wav", slot - 2);
    else out[0] = '\0';
}

String soundMapSlotLabel(int slot) {
    if (slot == 0) return "Loop";
    if (slot == 1) return "Standgas";
    if (slot == 2) return "Start";
    return "Sound " + String(slot - 2);
}

String soundMapSlotKey(int slot) {
    if (slot == 0) return "loop";
    if (slot == 1) return "shut";
    if (slot == 2) return "start";
    return String(slot - 2);
}

int soundMapSlotFromKey(const String& key) {
    if (key == "loop")  return 0;
    if (key == "shut")  return 1;
    if (key == "start") return 2;
    if (key.length() == 0 || key.length() > 2) return -1;
    for (size_t i = 0; i < key.length(); i++) if (!isDigit(key[i])) return -1;
    int n = key.toInt();
    return (n >= 1 && n <= RCSOUND_NUM_SOUNDS) ? n + 2 : -1;
}

static bool endsWithWav(const char* name, size_t len) {
    return len > 4 && strcasecmp(name + len - 4, ".wav") == 0;
}

bool soundNameOk(const char* name, bool strict) {
    if (!name) return false;
    size_t len = strlen(name);
    if (len >= SND_FILE_LEN || !endsWithWav(name, len) || name[0] == '.') return false;
    size_t base = len - 4;
    if (strict && (base > SND_NAME_MAX_BASE || strcmp(name + base, ".wav") != 0)) return false;
    for (size_t i = 0; i < base; i++) {
        unsigned char c = (unsigned char)name[i];
        if (strict) {
            if (!(isalnum(c) || c == '_' || c == '-')) return false;
        } else {
            if (c < 0x20 || c == '/' || c == '\\' || c == ';' || c == '"') return false;
        }
    }
    return true;
}

XT_Wav_Class* soundMapObject(int slot) {
    if (slot == 0) return &Sound_loop;
    if (slot == 1) return &Sound_shut;
    if (slot == 2) return &Sound_start;
    if (slot >= 3 && slot < SND_FILE_SLOTS) return Sounds[slot - 2];
    return nullptr;
}

// Leerer Platz: FileName "" (OpenWavFile() meldet dann sofort "keine Datei")
static String pathFor(const char* name) { return name[0] ? String("/") + name : String(""); }

void soundMapApplyAll() {
    for (int i = 0; i < SND_FILE_SLOTS; i++) {
        XT_Wav_Class* s = soundMapObject(i);
        if (s) s->FileName = pathFor(config.Sound_File[i]);
    }
}

// Haelt den Sound an und schliesst die Datei. true = lief gerade.
static bool stopAndClose(XT_Wav_Class* s) {
    bool wasPlaying = s->Playing;
    if (wasPlaying) I2SAudio.Stop(s);
    s->UnLoadWavFile();
    return wasPlaying;
}

bool soundMapSet(int slot, const char* name) {
    if (slot < 0 || slot >= SND_FILE_SLOTS || !name) return false;
    if (name[0] && !soundNameOk(name, false)) return false;
    if (strcmp(config.Sound_File[slot], name) == 0) return true;
    XT_Wav_Class* s = soundMapObject(slot);
    bool wasPlaying = s ? stopAndClose(s) : false;
    snprintf(config.Sound_File[slot], SND_FILE_LEN, "%s", name);
    if (s) {
        s->FileName = pathFor(name);
        // Motor-Dateien bleiben dauerhaft offen (LoadFiles() beim Start),
        // Sound 1-24 oeffnet handleSound() bei jedem Ausloesen selbst.
        if (slot < 3) {
            s->LoadWavFile();
            // Lief der Motor gerade, den Leerlauf mit der neuen Datei weiterspielen
            if (wasPlaying && slot == 0 && engine_State == 2 /* RUNNING */ && s->FileOK) {
                s->RepeatForever = true;
                I2SAudio.Play(s);
            }
        }
    }
    markDirty();
    Serial.printf("[Sound-Zuordnung] %s -> %s\n", soundMapSlotLabel(slot).c_str(), name[0] ? name : "(leer)");
    return true;
}

void soundMapCloseUsers(const char* name) {
    for (int i = 0; i < SND_FILE_SLOTS; i++)
        if (strcasecmp(config.Sound_File[i], name) == 0) {
            XT_Wav_Class* s = soundMapObject(i);
            if (s) stopAndClose(s);
        }
}

void soundMapReloadUsers(const char* name) {
    for (int i = 0; i < 3; i++)
        if (strcasecmp(config.Sound_File[i], name) == 0) soundMapObject(i)->LoadWavFile();
}

void soundMapClearUsers(const char* name) {
    for (int i = 0; i < SND_FILE_SLOTS; i++)
        if (strcasecmp(config.Sound_File[i], name) == 0) soundMapSet(i, "");
}

// SD-Karte ueber das VFS ("/sd", SD.begin() mit Standardpfad). Zugriff unter
// dem SD-Mutex wie die Sound-Wiedergabe; ist er belegt (z. B. waehrend eines
// Uploads), liefert die Liste nichts statt zu warten.
static const char* SD_ROOT = "/sd";
static bool sdReady() { return SD.cardType() != CARD_NONE; }

int soundListFiles(SoundFileInfo* out, int max) {
    int n = 0;
    if (!sdReady() || !sdMutexTake(500)) return 0;
    String root = SD_ROOT;
    DIR* d = opendir(root.c_str());
    if (!d) { sdMutexGive(); return 0; }
    struct dirent* e;
    while ((e = readdir(d)) != nullptr && n < max) {
        if (e->d_type == DT_DIR) continue;
        if (!soundNameOk(e->d_name, false)) continue;
        struct stat st;
        String full = root + "/" + e->d_name;
        if (stat(full.c_str(), &st) != 0) continue;
        snprintf(out[n].name, SND_FILE_LEN, "%s", e->d_name);
        out[n].size = (uint32_t)st.st_size;
        n++;
    }
    closedir(d);
    sdMutexGive();
    // alphabetisch (Gross-/Kleinschreibung egal), einfaches Einfuegesortieren
    for (int i = 1; i < n; i++) {
        SoundFileInfo t = out[i];
        int j = i - 1;
        while (j >= 0 && strcasecmp(out[j].name, t.name) > 0) { out[j + 1] = out[j]; j--; }
        out[j + 1] = t;
    }
    return n;
}

bool soundFileSize(const char* name, uint32_t& size) {
    if (!name || !name[0] || !sdReady() || !sdMutexTake(500)) return false;
    String full = String(SD_ROOT) + "/" + name;
    struct stat st;
    bool ok = stat(full.c_str(), &st) == 0;
    sdMutexGive();
    if (!ok) return false;
    size = (uint32_t)st.st_size;
    return true;
}
