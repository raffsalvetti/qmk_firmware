/* Copyright 2021 raffsalvetti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H

const char OLED_STATUS_NONE[] = "    ";

//http://blog.komar.be/how-to-make-a-keyboard-the-matrix/

// Defines names for use in layer keycodes and the keymap
enum layer_names {
    _BASE,
    _FN,
    _NUMPAD,
    _RGB
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* Base */
    [_BASE] = LAYOUT(
#ifdef LEFT_HAND
        KC_GRV,         KC_1,           KC_2,           KC_3,           KC_4,           KC_5,
        KC_TAB,         KC_Q,           KC_W,           KC_E,           KC_R,           KC_T,
        KC_CAPS,        KC_A,           KC_S,           KC_D,           KC_F,           KC_G,
        KC_LSFT,        KC_Z,           KC_X,           KC_C,           KC_V,           KC_B,
        KC_LCTL,        KC_BSLS,        KC_LGUI,        KC_LALT,        MO(_FN),
                                                                                                        KC_HOME,        KC_ESC,
                                                                                                                        TO(_FN),
                                                                                        KC_SPC,         KC_ENT,         TO(_BASE)
#elif RIGHT_HAND
                                        KC_6,           KC_7,           KC_8,           KC_9,           KC_0,           KC_MINS,
                                        KC_Y,           KC_U,           KC_I,           KC_O,           KC_P,           KC_BSPC,
                                        KC_H,           KC_J,           KC_K,           KC_L,           KC_SCLN,        KC_QUOT,
                                        KC_N,           KC_M,           KC_COMM,        KC_DOT,         KC_SLSH,        KC_RSFT,
                                                        KC_EQL,         KC_SCLN,        KC_LBRC,        KC_RBRC,        KC_RCTL,
        KC_DEL,         KC_END,
        KC_PGUP,
        KC_PGDN,        KC_ENT,         KC_SPC
#else
#error "Either LEFT_HAND or RIGHT_HAND should be define"
#endif
    ),
    [_FN] = LAYOUT(
#ifdef LEFT_HAND
        KC_NO,          KC_F1,          KC_F3,          KC_F4,          KC_F5,          KC_F6,
        KC_TRNS,        KC_MPRV,        KC_MPLY,        KC_MNXT,        KC_MSTP,        KC_VOLU,
        KC_TRNS,        KC_TRNS,        KC_MSTP,        KC_TRNS,        KC_BRIU,        KC_VOLD,
        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_BRID,        KC_MUTE,
        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,
                                                                                                        KC_TRNS,        KC_TRNS,
                                                                                                                        TO(_NUMPAD),
                                                                                        KC_TRNS,        KC_TRNS,        KC_TRNS
#elif RIGHT_HAND
                                        KC_F6,          KC_F7,          KC_F8,          KC_F9,          KC_F10,         KC_F11,
                                        KC_PSCR,        KC_PGUP,        KC_I,           KC_PGDN,        KC_TRNS,        KC_F12,
                                        KC_TRNS,        KC_HOME,        KC_UP,          KC_END,         KC_TRNS,        KC_TRNS,
                                        KC_TRNS,        KC_LEFT,        KC_DOWN,        KC_RGHT,        KC_TRNS,        KC_TRNS,
                                                        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,
        KC_PAUS,        KC_INS,
        KC_TRNS,
        KC_TRNS,        KC_TRNS,        KC_TRNS
#else
       #error "Either LEFT_HAND or RIGHT_HAND should be define"
#endif
    ),
    [_NUMPAD] = LAYOUT(
#ifdef LEFT_HAND
        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,
        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,
        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,
        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,
        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,
                                                                                                        KC_TRNS,        KC_TRNS,
                                                                                                                        TO(_RGB),
                                                                                        KC_TRNS,        KC_TRNS,        TO(_FN)
#elif RIGHT_HAND
                                        KC_NO,          KC_PSLS,        KC_PAST,        KC_PMNS,        KC_NO,          KC_NO,
                                        KC_NO,          KC_P7,          KC_P8,          KC_P9,          KC_NO,          KC_BSPC,
                                        KC_NO,          KC_P4,          KC_P5,          KC_P6,          KC_NO,          KC_NO,
                                        KC_NO,          KC_P1,          KC_P2,          KC_P3,          KC_NO,          KC_NO,
                                                        KC_P0,          KC_PDOT,        KC_NO,          KC_NO,          KC_NO,
        KC_CALC,        KC_ESC,
        KC_COPY,
        KC_PSTE,        KC_PENT,        KC_PPLS
#else
#error "Either LEFT_HAND or RIGHT_HAND should be define"
#endif
    ),
    [_RGB] = LAYOUT(
#ifdef LEFT_HAND
        RGB_M_SN,       RGB_M_X,        KC_NO,          KC_NO,          KC_NO,          KC_NO,
        RGB_M_SW,       RGB_RMOD,       RGB_TOG,        RGB_MOD,        KC_NO,          RGB_VAI,
        RGB_M_R,        KC_NO,          KC_NO,          KC_NO,          RGB_HUI,        RGB_VAD,
        RGB_M_B,        KC_NO,          KC_NO,          KC_NO,          RGB_HUD,        KC_TRNS,
        RGB_M_P,        KC_NO,          KC_NO,          KC_NO,          KC_NO,
                                                                                                        KC_NO,          KC_TRNS,
                                                                                                                        TO(_NUMPAD),
                                                                                        KC_NO,          KC_NO,          KC_NO
#elif RIGHT_HAND
                                        KC_NO,          KC_NO,          KC_NO,          KC_NO,          KC_NO,          KC_NO,
                                        KC_NO,          KC_NO,          KC_NO,          KC_NO,          KC_NO,          KC_NO,
                                        KC_NO,          KC_NO,          KC_NO,          KC_NO,          KC_NO,          KC_NO,
                                        KC_NO,          KC_NO,          KC_NO,          KC_NO,          KC_NO,          KC_NO,
                                                        KC_NO,          KC_NO,          KC_NO,          KC_NO,          KC_NO,
        KC_NO,          KC_NO,
        KC_NO,
        KC_NO,          KC_NO,          KC_NO
#else
#error "Either LEFT_HAND or RIGHT_HAND should be define"
#endif
    )
};

#ifdef OLED_DRIVER_ENABLE

// static void render_logo(void) {
//     static const char PROGMEM qmk_logo[] = {
//         0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94,
//         0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4,
//         0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0x00
//     };

//     oled_write_P(qmk_logo, false);
// }


void oled_task_user(void) {
    // Host Keyboard Layer Status
    oled_write_P(PSTR("Layer: "), false);

    switch (get_highest_layer(layer_state)) {
        case _BASE:
            oled_write_ln_P(PSTR("Base"), false);
            break;
        case _FN:
            oled_write_ln_P(PSTR("Fn"), false);
            break;
        case _RGB:
            oled_write_ln_P(PSTR("RGB"), false);
            break;
        default:
            // Or use the write_ln shortcut over adding '\n' to the end of your string
            oled_write_ln_P(PSTR("NAL"), false);
            // render_logo();
    }

    // Host Keyboard LED Status
    led_t led_state = host_keyboard_led_state();
    oled_write_P(led_state.num_lock ? PSTR("NUM ") : PSTR(OLED_STATUS_NONE), false);
    oled_write_P(led_state.caps_lock ? PSTR("CAP ") : PSTR(OLED_STATUS_NONE), false);
    oled_write_P(led_state.scroll_lock ? PSTR("SCR ") : PSTR(OLED_STATUS_NONE), false);
}

#endif
