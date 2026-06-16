#include QMK_KEYBOARD_H

/* =========================================================================
 * P0 skeleton keymap — single BASE layer, all keys transparent.
 *
 * QK_BOOT at (row 0, col 0) enables re-flashing once the USBasp-loader
 * bootloader is installed.
 *
 * Layout: 5 rows × 7 cols = 35 keys (single left-hand half keyboard)
 *
 * This keymap is fully populated in P1 (single-half keycodes) and P7
 * (complete 4-layer keymap with gaming macros).
 * ========================================================================= */

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_ESC,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    MACRO_1,
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    MACRO_2,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    MACRO_3,
        KC_LCTL, KC_LGUI, KC_LALT, MO(_LOWER), MO(_RAISE), KC_SPC , MACRO_4
    ),
    [_LOWER] = LAYOUT(
        KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_GRV,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, MO(_ADJUST), KC_TRNS, KC_TRNS
    ),
    [_RAISE] = LAYOUT(
        KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_TILD,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, MO(_ADJUST), KC_TRNS, KC_TRNS, KC_TRNS
    ),
    [_ADJUST] = LAYOUT(
        QK_BOOT, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        RM_TOGG, RM_NEXT, RM_HUEU, RM_SATU, RM_VALU, KC_TRNS, MS_SPD_1,
        LDR_TOG, RM_PREV, RM_HUED, RM_SATD, RM_VALD, KC_TRNS, MS_SPD_2,
        EEPROM_TEST, KC_TRNS, KC_TRNS, KC_TRNS, TG(_MUSIC), KC_TRNS, MS_SPD_3,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
    [_MUSIC] = LAYOUT(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, TG(_MUSIC),
        KC_TRNS, KC_TRNS, MU_C5,   MU_D5,   MU_E5,   KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, MU_F5,   MU_G5,   MU_A5,   KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, MU_B5,   MU_C6,   KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    )
};
