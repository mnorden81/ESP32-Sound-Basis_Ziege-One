#include "port_registry.h"
#include <cstring>

namespace port_registry {

// Reihenfolge == numerischer PortBMode-Wert - muss zu PORTB_MODULES[] in
// src/ESP32-RC-Sound/port_function.cpp passen.
static const char* const PORTB_NAMES[PORTB_FUNCTION_COUNT] = {
    "Aus", "CRSF", "S.Port", "Hott (bald)", "Raw", "GPS",
};

const char* const* portBFunctionNames() {
    return PORTB_NAMES;
}

const char* portBFunctionName(PortBMode f) {
    uint8_t idx = (uint8_t)f;
    if (idx >= PORTB_FUNCTION_COUNT) idx = (uint8_t)PORTB_OFF;
    return PORTB_NAMES[idx];
}

const char* portBFunctionOptionsString() {
    static char buf[64] = {0};
    if (buf[0] == '\0') {
        for (uint8_t i = 0; i < PORTB_FUNCTION_COUNT; i++) {
            if (i > 0) strncat(buf, ";", sizeof(buf) - strlen(buf) - 1);
            strncat(buf, PORTB_NAMES[i], sizeof(buf) - strlen(buf) - 1);
        }
    }
    return buf;
}

PortBMode clampPortBMode(int raw) {
    if (raw < 0 || raw >= (int)PORTB_FUNCTION_COUNT) return PORTB_OFF;
    return (PortBMode)raw;
}

static const uint32_t PORTB_BAUD_LIST[PORTB_BAUD_LIST_COUNT] = {
    9600, 19200, 38400, 57600, 115200, 230400, 420000
};

const uint32_t* portBBaudList() {
    return PORTB_BAUD_LIST;
}

const char* portBBaudListString() {
    return "9600;19200;38400;57600;115200;230400;420000";
}

uint8_t portBBaudToIndex(uint32_t baud) {
    for (uint8_t i = 0; i < PORTB_BAUD_LIST_COUNT; i++) {
        if (PORTB_BAUD_LIST[i] == baud) return i;
    }
    uint8_t best = 0;
    uint32_t bestDiff = 0xFFFFFFFFu;
    for (uint8_t i = 0; i < PORTB_BAUD_LIST_COUNT; i++) {
        uint32_t diff = (baud > PORTB_BAUD_LIST[i]) ? (baud - PORTB_BAUD_LIST[i]) : (PORTB_BAUD_LIST[i] - baud);
        if (diff < bestDiff) { bestDiff = diff; best = i; }
    }
    return best;
}

} // namespace port_registry
