#pragma once

#include "quantum.h"

void debug_led(uint8_t count, uint8_t r, uint8_t g, uint8_t b);

/* =========================================================================
 * Layer indices
 * ========================================================================= */
enum mercurio_gamer_layers {
    _BASE,
    _LOWER,
    _RAISE,
    _ADJUST,
    _MUSIC,
};

/* =========================================================================
 * Custom keycodes
 *
 * Populated in P7. Defined here so the enum exists from P0 onward.
 * ========================================================================= */
enum mercurio_gamer_keycodes {
    LDR_TOG = SAFE_RANGE,   /* Toggle LDR auto-dimming (P4)              */
    MS_SPD_1,                /* PS/2 mouse sensitivity 1                 */
    MS_SPD_2,                /* PS/2 mouse sensitivity 2                 */
    MS_SPD_3,                /* PS/2 mouse sensitivity 3                 */
    MACRO_1,                 /* Gaming macro slot 1 (P7)                  */
    MACRO_2,                 /* Gaming macro slot 2 (P7)                  */
    MACRO_3,                 /* Gaming macro slot 3 (P7)                  */
    MACRO_4,                 /* Gaming macro slot 4 (P7)                  */
    EEPROM_TEST,             /* 24LC1025 read/write test key (P6)         */
    MU_C5,                   /* Music Note: C5                            */
    MU_D5,                   /* Music Note: D5                            */
    MU_E5,                   /* Music Note: E5                            */
    MU_F5,                   /* Music Note: F5                            */
    MU_G5,                   /* Music Note: G5                            */
    MU_A5,                   /* Music Note: A5                            */
    MU_B5,                   /* Music Note: B5                            */
    MU_C6,                   /* Music Note: C6 (if reachable)             */
};
