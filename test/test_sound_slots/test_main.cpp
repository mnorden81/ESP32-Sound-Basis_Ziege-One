/*
 * test/test_sound_slots/test_main.cpp  –  ESP32-RC-Sound
 *
 * Sichert die CRSF-Feld-ID-Arithmetik rund um die Sound-Slots ab (siehe
 * lib/sound_slots/sound_slots.h fuer den Hintergrund), laeuft unter dem
 * PlatformIO "native"-Environment auf dem Entwicklungsrechner (kein ESP32
 * noetig):
 *
 *   pio test -e native
 *
 * WICHTIG: das hier ist kein Ersatz fuer den vollstaendigen Grep von
 * ESP32-RC-Sound.ino nach hartkodierten Sound-Anzahl-Literalen vor jeder
 * Aenderung - loop()/Config() haben eigene, von dieser Arithmetik
 * unabhaengige Schleifengrenzen, die dieses Modul nicht sieht.
 */

#include <unity.h>
#include "sound_slots.h"

using namespace sound_slots;

void setUp(void) {}
void tearDown(void) {}

// ── Sound-Formel ────────────────────────────────────────────────────────

static void test_first_and_last_sound_folder_id(void) {
    TEST_ASSERT_EQUAL_INT(16,  soundFolderId(1));
    TEST_ASSERT_EQUAL_INT(202, soundFolderId(kSoundCount));
}

static void test_sound_folder_ids_are_6_apart(void) {
    for (int s = 2; s <= kSoundCount; s++) {
        TEST_ASSERT_EQUAL_INT(kFieldsPerSound, soundFolderId(s) - soundFolderId(s - 1));
    }
}

static void test_last_sound_field_id(void) {
    // Sound 32, letztes der 6 Felder (Folder..Test): 16+(32-1)*6+5 = 207 -
    // der Wert, an dem die Sound-Bereichspruefung in crsfSendParam()/
    // crsfWriteParam() (idx<=...) enden muss.
    TEST_ASSERT_EQUAL_INT(207, lastSoundFieldId());
}

// ── Einstellungen-Block / Gesamt-Parameterzahl ─────────────────────────

static void test_settings_folder_starts_right_after_sounds(void) {
    TEST_ASSERT_EQUAL_INT(208, settingsFolderId());
}

static void test_param_count_covers_full_settings_block(void) {
    // 208 (Folder) .. 222 (Port C) = 15 lueckenlose Feld-IDs.
    TEST_ASSERT_EQUAL_INT(222, paramCount());
    TEST_ASSERT_EQUAL_INT(kSettingsBlockFieldCount, paramCount() - settingsFolderId() + 1);
}

// ── Erinnerung an den manuellen Sync-Punkt ─────────────────────────────

static void test_sound_count_is_32_as_currently_shipped(void) {
    // Faellt absichtlich auf, wenn RCSOUND_NUM_SLOTS (config.h) geaendert
    // wird, ohne kSoundCount hier (und die Feld-IDs in ESP32-RC-Sound.ino)
    // nachzuziehen - siehe Kommentar in sound_slots.h.
    TEST_ASSERT_EQUAL_INT(32, kSoundCount);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_first_and_last_sound_folder_id);
    RUN_TEST(test_sound_folder_ids_are_6_apart);
    RUN_TEST(test_last_sound_field_id);

    RUN_TEST(test_settings_folder_starts_right_after_sounds);
    RUN_TEST(test_param_count_covers_full_settings_block);

    RUN_TEST(test_sound_count_is_32_as_currently_shipped);

    return UNITY_END();
}
