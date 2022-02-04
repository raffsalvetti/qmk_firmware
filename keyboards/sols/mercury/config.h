/*
Copyright 2021 raffsalvetti

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define VENDOR_ID    0xFEED
#define PRODUCT_ID   0x1984
#define DEVICE_VER   0x0001
#define MANUFACTURER raffsalvetti
#define PRODUCT      sols.mercury

/* key matrix size */
#define MATRIX_ROWS 5
#define MATRIX_COLS 7

/*
 * Keyboard Matrix Assignments
 *
 * Change this to how you wired your keyboard
 * COLS: AVR pins used for columns, left to right
 * ROWS: AVR pins used for rows, top to bottom
 * DIODE_DIRECTION: COL2ROW = COL = Anode (+), ROW = Cathode (-, marked on diode)
 *                  ROW2COL = ROW = Anode (+), COL = Cathode (-, marked on diode)
 *
 */
#define MATRIX_ROW_PINS { A4, A3, A2, A1, A0 }
#define MATRIX_COL_PINS { C2, C3, C4, C5, C6, C7, A7 }
#define UNUSED_PINS

/* COL2ROW, ROW2COL */
#define DIODE_DIRECTION COL2ROW

#define SPLIT_USB_DETECT
#define SPLIT_USB_TIMEOUT 2000
#define SPLIT_USB_TIMEOUT_POLL 10

#define SPLIT_LAYER_STATE_ENABLE
#define SPLIT_LED_STATE_ENABLE
#define SPLIT_MODS_ENABLE
#define SPLIT_OLED_ENABLE

#define SOFT_SERIAL_PIN D2
#define SELECT_SOFT_SERIAL_SPEED 1 // or 0, 2, 3, 4, 5
                                   //  0: about 189kbps (Experimental only)
                                   //  1: about 137kbps (default)
                                   //  2: about 75kbps
                                   //  3: about 39kbps
                                   //  4: about 26kbps
                                   //  5: about 20kbps

//#define LED_NUM_LOCK_PIN B0
//#define LED_CAPS_LOCK_PIN B1
//#define LED_SCROLL_LOCK_PIN B2
//#define LED_COMPOSE_PIN B3
//#define LED_KANA_PIN B4

//#define BACKLIGHT_PIN B7
//#define BACKLIGHT_LEVELS 3
//#define BACKLIGHT_BREATHING

#ifdef RGBLIGHT_ENABLE
#    define RGB_DI_PIN B4
#    define RGBLED_NUM 35
#    define RGBLIGHT_HUE_STEP 8
#    define RGBLIGHT_SAT_STEP 8
#    define RGBLIGHT_VAL_STEP 8
#    define RGBLIGHT_LIMIT_VAL 160 /* The maximum brightness level */
//#    define RGBLIGHT_DEFAULT_MODE RGBLIGHT_EFFECT_BREATHING
#    define RGBLIGHT_SLEEP  /* If defined, the RGB lighting will be switched off when the host goes to sleep */
/*== all animations enable ==*/
#    define RGBLIGHT_ANIMATIONS
/*== or choose animations ==*/
#    define RGBLIGHT_EFFECT_BREATHING
#    define RGBLIGHT_EFFECT_RAINBOW_MOOD
#    define RGBLIGHT_EFFECT_RAINBOW_SWIRL
#    define RGBLIGHT_EFFECT_SNAKE
#    define RGBLIGHT_EFFECT_KNIGHT
#    define RGBLIGHT_EFFECT_CHRISTMAS
#    define RGBLIGHT_EFFECT_STATIC_GRADIENT
#    define RGBLIGHT_EFFECT_RGB_TEST
#    define RGBLIGHT_EFFECT_ALTERNATING
/*== customize breathing effect ==*/
/*==== (DEFAULT) use fixed table instead of exp() and sin() ====*/
#    define RGBLIGHT_BREATHE_TABLE_SIZE 256      // 256(default) or 128 or 64
/*==== use exp() and sin() ====*/
#    define RGBLIGHT_EFFECT_BREATHE_CENTER 1.85  // 1 to 2.7
#    define RGBLIGHT_EFFECT_BREATHE_MAX    255   // 0 to 255
#endif

/* Debounce reduces chatter (unintended double-presses) - set 0 if debouncing is not needed */
#define DEBOUNCE 5

/* Mechanical locking support. Use KC_LCAP, KC_LNUM or KC_LSCR instead in keymap */
#define LOCKING_SUPPORT_ENABLE
/* Locking resynchronize hack */
#define LOCKING_RESYNC_ENABLE

/* If defined, GRAVE_ESC will always act as ESC when CTRL is held.
 * This is useful for the Windows task manager shortcut (ctrl+shift+esc).
 */
//#define GRAVE_ESC_CTRL_OVERRIDE

/*
 * Force NKRO
 *
 * Force NKRO (nKey Rollover) to be enabled by default, regardless of the saved
 * state in the bootmagic EEPROM settings. (Note that NKRO must be enabled in the
 * makefile for this to work.)
 *
 * If forced on, NKRO can be disabled via magic key (default = LShift+RShift+N)
 * until the next keyboard reset.
 *
 * NKRO may prevent your keystrokes from being detected in the BIOS, but it is
 * fully operational during normal computer usage.
 *
 * For a less heavy-handed approach, enable NKRO via magic key (LShift+RShift+N)
 * or via bootmagic (hold SPACE+N while plugging in the keyboard). Once set by
 * bootmagic, NKRO mode will always be enabled until it is toggled again during a
 * power-up.
 *
 */
//#define FORCE_NKRO

/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

/* disable debug print */
// #define NO_DEBUG

/* disable print */
// #define NO_PRINT

/* disable action features */
//#define NO_ACTION_LAYER
//#define NO_ACTION_TAPPING
//#define NO_ACTION_ONESHOT

/* disable these deprecated features by default */
#define NO_ACTION_MACRO
#define NO_ACTION_FUNCTION

/* Bootmagic Lite key configuration */
#define BOOTMAGIC_LITE_ROW 1
#define BOOTMAGIC_LITE_COLUMN 6


//configuracoes v-usb
#define USB_CFG_IOPORTNAME D
#define USB_CFG_DMINUS_BIT 3
#define USB_CFG_DPLUS_BIT 4

#define USB_INTR_CFG MCUCR
#define USB_INTR_CFG_SET ((1 << ISC11) | (0 << ISC10))
#define USB_INTR_CFG_CLR 0

#define USB_INTR_ENABLE_BIT INT1
#define USB_INTR_PENDING_BIT INTF1
#define USB_INTR_VECTOR INT1_vect

//configuracoes para mouse
#ifdef PS2_USE_INT
    #define PS2_MOUSE_INIT_DELAY 2000

    #define PS2_CLOCK_PORT  PORTD
    #define PS2_CLOCK_PIN   PIND
    #define PS2_CLOCK_DDR   DDRD
    #define PS2_CLOCK_BIT   2

    #define PS2_DATA_PORT   PORTD
    #define PS2_DATA_PIN    PIND
    #define PS2_DATA_DDR    DDRD
    #define PS2_DATA_BIT    5

    #define PS2_INT_INIT()  do { \
        MCUCR |= ((1 << ISC01) | (0 << ISC00)); \
    } while (0)

    #define PS2_INT_ON()  do { \
        GICR |= (1 << INT0); \
    } while (0)

    #define PS2_INT_OFF() do { \
        GICR &= ~(1 << INT0); \
    } while (0)

    #define PS2_INT_VECT INT0_vect
#endif

#ifdef OLED_DRIVER_ENABLE
    #define OLED_DISPLAY_128X64
    #define OLED_DISPLAY_ADDRESS    0x3C
    #define OLED_IC                 OLED_IC_SSD1306
#endif

#ifdef AUDIO_ENABLE
#   define AUDIO_PIN D5
#   define AUDIO_CLICKY
#endif


