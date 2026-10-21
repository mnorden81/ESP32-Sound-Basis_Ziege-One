// uart_roles.cpp - RC_Common, siehe uart_roles.h
#include "uart_roles.h"

static const char* const ROLE_NAMES[UROLE_COUNT] = {"Aus", "CRSF", "S.Port", "GPS", "Raw"};

const char* uartRoleName(uint8_t role) {
    return role < UROLE_COUNT ? ROLE_NAMES[role] : ROLE_NAMES[UROLE_OFF];
}

const char* uartRoleOptions() { return "Aus;CRSF;S.Port;GPS;Raw"; }

uint8_t uartRoleClamp(int raw) {
    return (raw >= 0 && raw < UROLE_COUNT) ? (uint8_t)raw : UROLE_OFF;
}

uint32_t uartRoleDefaultBaud(uint8_t role) {
    switch (role) {
        case UROLE_CRSF:  return 420000;
        case UROLE_SPORT: return 57600;
        case UROLE_GPS:   return 9600;
        case UROLE_RAW:   return 115200;
        default:          return 0;
    }
}

bool uartRoleSingleInstance(uint8_t role) {
    return role == UROLE_SPORT || role == UROLE_GPS || role == UROLE_RAW;
}

uint8_t uartRoleResolveSecond(uint8_t first, uint8_t second, bool* duplicate) {
    bool dup = (second == first) && uartRoleSingleInstance(second);
    if (duplicate) *duplicate = dup;
    return dup ? UROLE_OFF : second;
}

const uint32_t UART_BAUD_LIST[UART_BAUD_COUNT] = {9600, 19200, 38400, 57600, 115200, 230400, 420000};

const char* uartBaudOptions() { return "9600;19200;38400;57600;115200;230400;420000"; }

uint8_t uartBaudToIndex(uint32_t baud) {
    uint8_t best = 0;
    uint32_t bestDiff = 0xFFFFFFFFu;
    for (uint8_t i = 0; i < UART_BAUD_COUNT; i++) {
        uint32_t d = baud > UART_BAUD_LIST[i] ? baud - UART_BAUD_LIST[i] : UART_BAUD_LIST[i] - baud;
        if (d < bestDiff) { bestDiff = d; best = i; }
    }
    return best;
}

uint32_t uartBaudFromIndex(uint8_t index) {
    return UART_BAUD_LIST[index < UART_BAUD_COUNT ? index : UART_BAUD_COUNT - 1];
}

uint8_t uartRoleFromSoundBasisLegacy(int legacy) {
    switch (legacy) {
        case 1:  return UROLE_CRSF;
        case 2:  return UROLE_SPORT;
        case 4:  return UROLE_RAW;
        case 5:  return UROLE_GPS;
        default: return UROLE_OFF;   // 0 Aus, 3 Hott (nie umgesetzt), ungueltig
    }
}

uint8_t uartRoleFromSoundMiniLegacy(int legacy) {
    switch (legacy) {
        case 1:  return UROLE_CRSF;
        case 2:  return UROLE_SPORT;
        case 3:  return UROLE_RAW;
        case 4:  return UROLE_GPS;
        default: return UROLE_OFF;
    }
}

uint8_t uartRoleFromMultiswitchLegacy(int legacy) {
    return (legacy >= 0 && legacy <= 3) ? (uint8_t)legacy : UROLE_OFF;
}
