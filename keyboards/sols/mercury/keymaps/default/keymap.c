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
#include "raw_hid.h"

const char OLED_STATUS_NONE[] = "    ";

//http://blog.komar.be/how-to-make-a-keyboard-the-matrix/

// Defines names for use in layer keycodes and the keymap
enum layer_names {
    _BASE
    ,_FN
    ,_NUMPAD
#ifdef RGBLIGHT_ENABLE
    ,_RGB
#endif
};

#ifdef RGBLIGHT_ENABLE
#define RGB_KEY_TRIGGER TO(_RGB)
#else
#define RGB_KEY_TRIGGER KC_NO
#endif

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
    )
    ,[_FN] = LAYOUT(
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
    )
    ,[_NUMPAD] = LAYOUT(
#ifdef LEFT_HAND
        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,
        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,
        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,
        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,
        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,        KC_TRNS,
                                                                                                        KC_TRNS,        KC_TRNS,
                                                                                                                        RGB_KEY_TRIGGER,
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
    )
#ifdef RGBLIGHT_ENABLE
    ,[_RGB] = LAYOUT(
#ifdef LEFT_HAND
        RGB_M_SN,       RGB_M_X,        KC_NO,          KC_NO,          KC_NO,          KC_NO,
        RGB_M_SW,       RGB_RMOD,       RGB_TOG,        RGB_MOD,        KC_NO,          RGB_VAI,
        RGB_M_R,        KC_NO,          KC_NO,          KC_NO,          RGB_HUI,        RGB_VAD,
        RGB_M_B,        KC_NO,          KC_NO,          KC_NO,          RGB_HUD,        KC_TRNS,
        RGB_M_P,        KC_NO,          KC_NO,          KC_NO,          KC_NO,
                                                                                                        KC_NO,          KC_TRNS,
                                                                                                                        KC_NO,
                                                                                        KC_NO,          KC_NO,          TO(_NUMPAD)
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
#endif
};

#ifdef OLED_DRIVER_ENABLE

// static void render_logo(void) {
//     static const char PROGMEM logo[] = {
//         0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xf0, 0x7f, 0xff, 0xff, 0xe1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xe3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xfe, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xf3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0x9f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xde, 0x1f, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0f, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x0f, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x07, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x07, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x07, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x0b, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xf0, 0x1f, 0xff, 0xff, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0x07, 0xff, 0xff, 0xff, 0xfc, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff,
//         0xff, 0xf0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xe1, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff,
//         0xff, 0x87, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xff,
//         0xfe, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x87, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0xff,
//         0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xc7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xff, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff, 0xff, 0xff, 0xf7, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xff, 0xfb, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xff, 0xfd, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9f, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdc, 0xff, 0xff, 0xcf, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xf6, 0x06, 0x0f, 0x87, 0x7d, 0x83, 0xcc, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xff, 0x7f,
//         0xff, 0xf7, 0xe6, 0x06, 0x07, 0x03, 0x7d, 0x01, 0xc1, 0xff, 0xff, 0xfb, 0xff, 0xff, 0xff, 0xbf,
//         0xff, 0xf3, 0xc6, 0xfe, 0xf2, 0x7f, 0x7d, 0x3c, 0xc1, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xdf,
//         0xff, 0xf1, 0x86, 0xfe, 0x42, 0xff, 0x7d, 0x01, 0xdc, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
//         0xff, 0xf4, 0x16, 0x06, 0x06, 0xff, 0x7d, 0x01, 0xdc, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xef,
//         0xff, 0xf6, 0x76, 0xfe, 0xe6, 0xff, 0x7d, 0x39, 0xcc, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xff, 0xf7,
//         0xff, 0xf7, 0xf6, 0xfe, 0xe6, 0xff, 0x79, 0xb9, 0xc1, 0xff, 0xff, 0xff, 0xdf, 0xff, 0xff, 0xff,
//         0xff, 0xf7, 0xf6, 0xfe, 0xf2, 0x7f, 0x39, 0xbc, 0xe3, 0xff, 0xff, 0xff, 0xef, 0xff, 0xff, 0xfb,
//         0xff, 0xf7, 0xf6, 0x06, 0xfb, 0x03, 0x83, 0xbe, 0xe1, 0xff, 0xff, 0xff, 0xf7, 0xff, 0xff, 0xfd,
//         0xff, 0xf7, 0xf6, 0x06, 0xfb, 0xcf, 0xcf, 0xff, 0xe3, 0xff, 0xff, 0xff, 0xfb, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff, 0xff, 0xfe,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff
//     };

//     oled_write_P(logo, false);
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
        case _NUMPAD:
            oled_write_ln_P(PSTR("Num"), false);
            break;
#ifdef RGBLIGHT_ENABLE
        case _RGB:
            oled_write_ln_P(PSTR("RGB"), false);
            break;
#endif
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

// Happens before most anything is started.
// Good for hardware setup that you want running very early.
void keyboard_pre_init_user(void) {

}
 // Happens at the end of the firmware’s startup process.
 // This is where you’d want to put “customization” code, for the most part.
void keyboard_post_init_user(void) {

}

// eh chamado depois de executar todos os eventos do loop principal
// depois de ler as teclas e enviar os dados via udb, por ex
void housekeeping_task_user(void) {
    // if (is_keyboard_master()) {
    //     // Interact with slave every 500ms
    //     static uint32_t last_sync = 0;
    //     if (timer_elapsed32(last_sync) > 500) {
    //         master_to_slave_t m2s = {6};
    //         slave_to_master_t s2m = {0};
    //         if(transaction_rpc_exec(USER_SYNC_A, sizeof(m2s), &m2s, sizeof(s2m), &s2m)) {
    //             last_sync = timer_read32();
    //             dprintf("Slave value: %d\n", s2m.s2m_data); // this will now be 11, as the slave adds 5
    //         } else {
    //             dprint("Slave sync failed!\n");
    //         }
    //     }
    // }
}

// The keycode argument is whatever is defined in your keymap, eg MO(1), KC_L,
// etc. You should use a switch...case block to handle these events.
// The record argument contains information about the actual press:
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!is_keyboard_master()) {
        // enviar codigo da tecla para o master
        return false; //impede continuar o tratamento
    }
    return true; //true continua processnado o evento pelo core do qmk
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
    // Your code goes here. data is the packet received from host.

    //raw_hid_send(data, length);
}

// Use the IS_LAYER_ON_STATE(state, layer) and IS_LAYER_OFF_STATE(state, layer)
// macros to check the status of a particular layer.
// Outside of layer_state_set_* functions, you can use the IS_LAYER_ON(layer)
// and IS_LAYER_OFF(layer) macros to check global layer state.
// https://docs.qmk.fm/#/keymap?id=keymap-layer-status
layer_state_t layer_state_set_user(layer_state_t state) {
    return state;
}

// void transport_master_init(void) {

// }

// void transport_slave_init(void) {

// }

// bool transport_master(matrix_row_t master_matrix[], matrix_row_t slave_matrix[]) {
//     return 0;
// }

// void transport_slave(matrix_row_t master_matrix[], matrix_row_t slave_matrix[]) {

// }
