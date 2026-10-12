/*
 * test/test_native/test_main.cpp  –  ESP32-RC-Sound
 *
 * Unity-Tests fuer die hardwareunabhaengige Port-B-Logik
 * (lib/port_registry), laufen unter dem PlatformIO "native"-Environment
 * auf dem Entwicklungsrechner (kein ESP32 noetig):
 *
 *   pio test -e native
 */

#include <unity.h>
#include "port_registry.h"

using namespace port_registry;

void setUp(void) {}
void tearDown(void) {}

// ── PortBMode / Options-String ─────────────────────────────────────────

static void test_portb_function_count_matches_names(void) {
    // Wenn jemand PORTB_FUNCTION_COUNT erhoeht/verringert, ohne die Namens-
    // tabelle nachzuziehen, faellt das hier sofort auf (statt erst als Off-
    // by-one-Bug in der Options-String-Ausgabe).
    TEST_ASSERT_EQUAL_UINT8(5, PORTB_FUNCTION_COUNT);
}

static void test_portb_function_name_known_values(void) {
    TEST_ASSERT_EQUAL_STRING("Aus",         portBFunctionName(PORTB_OFF));
    TEST_ASSERT_EQUAL_STRING("CRSF",        portBFunctionName(PORTB_CRSF));
    TEST_ASSERT_EQUAL_STRING("S.Port",      portBFunctionName(PORTB_SPORT));
    TEST_ASSERT_EQUAL_STRING("Hott (bald)", portBFunctionName(PORTB_HOTT));
    TEST_ASSERT_EQUAL_STRING("Raw",         portBFunctionName(PORTB_RAW));
}

static void test_portb_function_name_out_of_range_falls_back_to_off(void) {
    TEST_ASSERT_EQUAL_STRING("Aus", portBFunctionName((PortBMode)99));
}

static void test_portb_function_options_string_matches_firmware(void) {
    // Muss exakt zu portBFunctionOptionsString() in
    // src/ESP32-RC-Sound/port_function.cpp passen (Index == PortBMode-Wert,
    // siehe fillSelFlat()/CRSF_TEXT_SELECTION im Web-UI bzw. CRSF/Lua-Menue).
    TEST_ASSERT_EQUAL_STRING("Aus;CRSF;S.Port;Hott (bald);Raw", portBFunctionOptionsString());
}

static void test_portb_function_max_index(void) {
    TEST_ASSERT_EQUAL_UINT8(4, portBFunctionMaxIndex());
}

// ── clampPortBMode() ────────────────────────────────────────────────────

static void test_clamp_portb_mode_valid_values_pass_through(void) {
    TEST_ASSERT_EQUAL(PORTB_OFF,   clampPortBMode(0));
    TEST_ASSERT_EQUAL(PORTB_CRSF,  clampPortBMode(1));
    TEST_ASSERT_EQUAL(PORTB_SPORT, clampPortBMode(2));
    TEST_ASSERT_EQUAL(PORTB_HOTT,  clampPortBMode(3));
    TEST_ASSERT_EQUAL(PORTB_RAW,   clampPortBMode(4));
}

static void test_clamp_portb_mode_out_of_range_falls_back_to_off(void) {
    TEST_ASSERT_EQUAL(PORTB_OFF, clampPortBMode(-1));
    TEST_ASSERT_EQUAL(PORTB_OFF, clampPortBMode(5));
    TEST_ASSERT_EQUAL(PORTB_OFF, clampPortBMode(255));
}

// ── Baudraten-Liste (CRSF/Lua-Menue) ────────────────────────────────────

static void test_baud_list_count_matches_string(void) {
    TEST_ASSERT_EQUAL_UINT8(7, PORTB_BAUD_LIST_COUNT);
    TEST_ASSERT_EQUAL_STRING("9600;19200;38400;57600;115200;230400;420000", portBBaudListString());
}

static void test_baud_to_index_exact_matches(void) {
    TEST_ASSERT_EQUAL_UINT8(0, portBBaudToIndex(9600));
    TEST_ASSERT_EQUAL_UINT8(1, portBBaudToIndex(19200));
    TEST_ASSERT_EQUAL_UINT8(2, portBBaudToIndex(38400));
    TEST_ASSERT_EQUAL_UINT8(3, portBBaudToIndex(57600));
    TEST_ASSERT_EQUAL_UINT8(4, portBBaudToIndex(115200));
    TEST_ASSERT_EQUAL_UINT8(5, portBBaudToIndex(230400));
    TEST_ASSERT_EQUAL_UINT8(6, portBBaudToIndex(420000));
}

static void test_baud_to_index_snaps_to_nearest(void) {
    // Frei ueber's Web eingegebener Wert, nicht in der Liste - Lua-Menue
    // zeigt den naechstgelegenen Eintrag (ohne den gespeicherten Wert zu
    // veraendern, das macht nur die Firmware-Seite, siehe port_function.cpp).
    TEST_ASSERT_EQUAL_UINT8(0, portBBaudToIndex(9601));      // knapp ueber 9600
    TEST_ASSERT_EQUAL_UINT8(3, portBBaudToIndex(60000));     // naeher an 57600 als 115200
    TEST_ASSERT_EQUAL_UINT8(4, portBBaudToIndex(100000));    // naeher an 115200 als 57600
    TEST_ASSERT_EQUAL_UINT8(6, portBBaudToIndex(500000));    // ueber der Liste -> hoechster Eintrag
    TEST_ASSERT_EQUAL_UINT8(0, portBBaudToIndex(0));         // unter der Liste -> niedrigster Eintrag
}

static void test_baud_to_index_midpoint_prefers_lower(void) {
    // Exakt in der Mitte zwischen 57600 und 115200: (57600+115200)/2 = 86400.
    // diff zu 57600 = 28800, diff zu 115200 = 28800 -> "<" in der Firmware-
    // Logik (kein "<="), also bleibt der ZUERST gefundene (niedrigere) Index
    // stehen. Fixiert dieses Verhalten explizit, statt es implizit offen zu lassen.
    TEST_ASSERT_EQUAL_UINT8(3, portBBaudToIndex(86400));
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_portb_function_count_matches_names);
    RUN_TEST(test_portb_function_name_known_values);
    RUN_TEST(test_portb_function_name_out_of_range_falls_back_to_off);
    RUN_TEST(test_portb_function_options_string_matches_firmware);
    RUN_TEST(test_portb_function_max_index);

    RUN_TEST(test_clamp_portb_mode_valid_values_pass_through);
    RUN_TEST(test_clamp_portb_mode_out_of_range_falls_back_to_off);

    RUN_TEST(test_baud_list_count_matches_string);
    RUN_TEST(test_baud_to_index_exact_matches);
    RUN_TEST(test_baud_to_index_snaps_to_nearest);
    RUN_TEST(test_baud_to_index_midpoint_prefers_lower);

    return UNITY_END();
}
