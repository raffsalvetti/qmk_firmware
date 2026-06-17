#include "rev1_gamer.h"
#include "ldr.h"
#include "piezo.h"
#include "piezo_effects.h"
#include "i2c_module.h"
#include "eeprom_ext.h"

#ifdef RGB_MATRIX_ENABLE
led_config_t g_led_config = {
    // Key Matrix to LED Index (Vertical Zigzag starting Top-Right)
    {
        { 30, 29, 20, 19, 10,  9,  0 },
        { 31, 28, 21, 18, 11,  8,  1 },
        { 32, 27, 22, 17, 12,  7,  2 },
        { 33, 26, 23, 16, 13,  6,  3 },
        { 34, 25, 24, 15, 14,  5,  4 }
    },
    // LED Index to Physical Position (Vertical Zigzag)
    {
        // Col 6 (Down)
        {222,   0}, {222,  16}, {222,  32}, {222,  48}, {222,  64},
        // Col 5 (Up)
        {185,  64}, {185,  48}, {185,  32}, {185,  16}, {185,   0},
        // Col 4 (Down)
        {148,   0}, {148,  16}, {148,  32}, {148,  48}, {148,  64},
        // Col 3 (Up)
        {111,  64}, {111,  48}, {111,  32}, {111,  16}, {111,   0},
        // Col 2 (Down)
        { 74,   0}, { 74,  16}, { 74,  32}, { 74,  48}, { 74,  64},
        // Col 1 (Up)
        { 37,  64}, { 37,  48}, { 37,  32}, { 37,  16}, { 37,   0},
        // Col 0 (Down)
        {  0,   0}, {  0,  16}, {  0,  32}, {  0,  48}, {  0,  64}
    },
    // LED Index to Flag (e.g. keylight)
    {
        4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4
    }
};
#endif

#ifdef MOUSE_ENABLE
    extern void custom_mouse_init(void);
    extern void custom_mouse_task(void);
#endif

/* =========================================================================
 * keyboard_pre_init_kb
 * ========================================================================= */
void keyboard_pre_init_kb(void) {
    keyboard_pre_init_user();
}

/* =========================================================================
 * matrix_init_kb
 * ========================================================================= */
void matrix_init_kb(void) {
    matrix_init_user();
}

/* =========================================================================
 * keyboard_post_init_kb
 *
 * Called once after QMK finishes its own initialisation.
 * Module init calls are added here in later phases:
 *   P4: ldr_init()
 *   P5: piezo_init(), piezo_effect_startup()
 *   P6: i2c_module_init(), eeprom_ext_init()
 * ========================================================================= */
void keyboard_post_init_kb(void) {
    ldr_init();
    piezo_init();
    piezo_effect_startup();
    
    i2c_module_init();
    
    eeprom_ext_init();
    
#ifdef MOUSE_ENABLE
    custom_mouse_init();
#endif

    keyboard_post_init_user();
}

/* =========================================================================
 * housekeeping_task_kb
 *
 * Called every scan cycle (~1 ms).  Rate-limited module tasks are
 * dispatched from here in later phases:
 *   P4: ldr_task()           (every 100 ms via timer_elapsed32)
 *   P5: piezo_task()         (every 1 ms)
 * ========================================================================= */
extern uint8_t usbConfiguration;

#include "i2c_master.h"

void housekeeping_task_kb(void) {
    static uint8_t last_usb_config = 0;
    
    if (usbConfiguration != last_usb_config) {
        last_usb_config = usbConfiguration;
        if (usbConfiguration) {
            // USB successfully enumerated and configured by the host OS!
            // debug_led(7, 255, 0, 255); // 7 Purple LEDs: USB OK!
        }
    }

    // --- Boot Timer Tracker ---
    // Tracks boot time for the OLED UI screen timeout.
    static uint32_t boot_timer = 0;
    if (boot_timer == 0) {
        boot_timer = timer_read32();
    }

#ifdef MOUSE_ENABLE
    // External custom_mouse_task from custom_mouse.c handles async init and parsing
    custom_mouse_task();
#endif
    
    ldr_task();
    piezo_task();
    piezo_effects_task();
    housekeeping_task_user();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    // if (record->event.pressed) {
    //     piezo_effect_keyclick();
    // }
    
    switch (keycode) {
        case LDR_TOG:
            // if (record->event.pressed) {
            //     ldr_toggle();
            // }
            return false;
        case EEPROM_TEST: {
            // if (record->event.pressed) {
                uint8_t test_val = 0;
                bool read_ok = eeprom_ext_read(0x0000, &test_val, 1);
                
                if (!read_ok) {
                    // Read failed! Show RED
                    // rgb_matrix_set_color(1, 0, 0, 0);
                    // rgb_matrix_set_color(0, 255, 0, 0); // Top-right RED
                    return false;
                }
                
                if(test_val == 0xAA) {
                    test_val = 0x55;
                    // rgb_matrix_set_color(1, 0, 0, 0);
                    // rgb_matrix_set_color(0, 0, 255, 0); // Green
                } else {
                    test_val = 0xAA;
                    // rgb_matrix_set_color(0, 0, 0, 0);
                    // rgb_matrix_set_color(1, 0, 255, 0); // Green
                }
                
                bool write_ok = eeprom_ext_write_page(0x0000, &test_val, 1);
                if (!write_ok) {
                    // Write failed! Show Purple
                    // rgb_matrix_set_color(1, 0, 0, 0);
                    // rgb_matrix_set_color(0, 255, 0, 255); // Top-right PURPLE
                }
            // }
            return false;
        }
        case MACRO_1:
            if (record->event.pressed) SEND_STRING("GLHF\n");
            return false;
        case MACRO_2:
            if (record->event.pressed) SEND_STRING("GGWP\n");
            return false;
        case MACRO_3:
            if (record->event.pressed) SEND_STRING("BRB\n");
            return false;
        case MACRO_4:
            if (record->event.pressed) SEND_STRING("TY\n");
            return false;
        case MU_C5:
        case MU_D5:
        case MU_E5:
        case MU_F5:
        case MU_G5:
        case MU_A5:
        case MU_B5:
        case MU_C6:
            if (record->event.pressed) {
                uint8_t note = PIEZO_NOTE_C5;
                if (keycode == MU_D5) note = PIEZO_NOTE_D5;
                else if (keycode == MU_E5) note = PIEZO_NOTE_E5;
                else if (keycode == MU_F5) note = PIEZO_NOTE_F5;
                else if (keycode == MU_G5) note = PIEZO_NOTE_G5;
                else if (keycode == MU_A5) note = PIEZO_NOTE_A5;
                else if (keycode == MU_B5) note = PIEZO_NOTE_B5;
                else if (keycode == MU_C6) note = 119; // ~1046 Hz
                
                piezo_play(note, 10000); // Play continuously until released
            } else {
                piezo_stop();
            }
            return false;
        case MS_SPD_1:
            // Placeholder for mouse speed config if needed
            return false;
        case MS_SPD_2:
            return false;
        case MS_SPD_3:
            return false;
    }
    return process_record_user(keycode, record);
}

layer_state_t layer_state_set_kb(layer_state_t state) {
    //piezo_effect_layer_change(get_highest_layer(state));
    return layer_state_set_user(state);
}

/* =========================================================================
 * PS/2 Mouse Debug Hook
 * ========================================================================= */
#ifdef PS2_MOUSE_ENABLE
void ps2_mouse_moved_user(report_mouse_t *mouse_report) {
    // If the mouse sends any movement data, flash the Top Right LED Blue
    // This proves the physical communication and interrupts are working!
    //rgb_matrix_set_color(0, 0, 0, 255);
}
#endif

/* =========================================================================
 * RGB Matrix Indicators Hook (Testing Key Press to LED Mapping)
 * ========================================================================= */
#ifdef RGB_MATRIX_ENABLE
bool rgb_matrix_indicators_user(void) {
    
    // // Turn off all LEDs to isolate pressed keys
    // for (uint8_t i = 0; i < 35; i++) {
    //     rgb_matrix_set_color(i, 0, 0, 0);
    // }
    
    // // Read the matrix and light up the exact LED assigned to the pressed key
    // for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
    //     for (uint8_t col = 0; col < MATRIX_COLS; col++) {
    //         if (matrix_is_on(row, col)) {
    //             uint8_t led_index = g_led_config.matrix_co[row][col];
    //             // Light it up bright Green!
    //             rgb_matrix_set_color(led_index, 0, 255, 0);
    //         }
    //     }
    // }
    
    return false;
}
#endif
/* =========================================================================
 * OLED Display UI
 * ========================================================================= */
#ifdef OLED_ENABLE
bool oled_task_user(void) {
    static uint32_t boot_timer = 0;
    static bool boot_screen_done = false;
    
    // State tracking variables to prevent I2C bus flooding
    static uint32_t last_layer_state = 0xFFFFFFFF;
    static uint8_t last_led_state = 0xFF;
    static uint8_t last_usb_config = 0xFF;

    if (boot_timer == 0) {
        boot_timer = timer_read32();
        
        // Draw boot screen
        oled_clear();
        oled_set_cursor(1, 1);
        oled_write_P(PSTR("===================="), false);
        oled_set_cursor(1, 2);
        oled_write_P(PSTR("   MERCURIO REV1-G  "), false);
        oled_set_cursor(1, 3);
        oled_write_P(PSTR("===================="), false);

        return false;
    }

    if (!boot_screen_done) {
        if (timer_elapsed32(boot_timer) > 2000) { // Keep the boot screen longer so user can see it!
            boot_screen_done = true;
            oled_clear();
        } else {
            return false; // Still showing boot screen
        }
    }

    // --- State-Driven Live Status UI ---
    uint32_t current_layer_state = layer_state;
    led_t current_led_state = host_keyboard_led_state();
    uint8_t current_usb_config = usbConfiguration;

    oled_set_cursor(0, 0);

    static uint16_t last_ldr_val = 0xFFFF;

    // Only update the display if something actually changed!
    if (current_layer_state != last_layer_state || 
        current_led_state.raw != last_led_state || 
        current_usb_config != last_usb_config ||
        ldr_adc_val != last_ldr_val) {
        
        last_layer_state = current_layer_state;
        last_led_state = current_led_state.raw;
        last_usb_config = current_usb_config;
        last_ldr_val = ldr_adc_val;

        // Draw Active Layer
        oled_set_cursor(1, 0);
        uint8_t layer = get_highest_layer(layer_state);
        oled_write_P(PSTR("Layer: "), false);
        switch (layer) {
            case 0:
                oled_write_P(PSTR("Default  "), false);
                break;
            case 1:
                oled_write_P(PSTR("Gaming   "), false);
                break;
            case 2:
                oled_write_P(PSTR("Fn/Media "), false);
                break;
            default:
                oled_write_P(PSTR("Unknown  "), false);
                break;
        }

        // Draw Lock States
        oled_set_cursor(1, 2);
        oled_write_P(current_led_state.caps_lock ? PSTR("[CAPS] ") : PSTR("       "), false);
        oled_write_P(current_led_state.num_lock  ? PSTR("[NUM]  ") : PSTR("       "), false);
        oled_write_P(current_led_state.scroll_lock ? PSTR("[SCR]  ") : PSTR("       "), false);
        
        // Draw USB Status
        oled_set_cursor(1, 3);
        if (current_usb_config) {
            oled_write_P(PSTR("USB: CONNECTED   "), false);
        } else {
            oled_write_P(PSTR("USB: WAITING...  "), false);
        }
        oled_set_cursor(1, 4);
        oled_write_P(PSTR("===================="), false);
        oled_set_cursor(1, 5);
        oled_write_P(PSTR("LDR RAW: "), false);
        oled_write(get_u16_str(ldr_adc_val, ' '), false);
    }
    
    return false; 
}
#endif
