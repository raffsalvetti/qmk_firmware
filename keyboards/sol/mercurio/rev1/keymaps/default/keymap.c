#include QMK_KEYBOARD_H

/* =========================================================================
 * P0 skeleton keymap — single BASE layer, all keys transparent.
 *
 * QK_BOOT at (row 0, col 0) enables re-flashing once the USBasp-loader
 * bootloader is installed (no ISP programmer needed after P10).
 *
 * This keymap is fully populated in P1 (single-half keycodes) and P9
 * (complete 4-layer split keymap).
 * ========================================================================= */

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        QK_BOOT, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
};
