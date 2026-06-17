#pragma once

/* =========================================================================
 * sol/mercurio rev1-gamer — QMK firmware configuration
 * MCU : ATmega32A, 44-pin TQFP
 * USB : V-USB, 16 MHz external crystal
 *
 * Left-hand half keyboard + PS/2 mouse (MX8731A)
 * No split keyboard — single board with 5×7 matrix (35 keys)
 *
 * USB identity (vid, pid, manufacturer, keyboard_name, device_version) is
 * defined in keyboard.json — do not duplicate here.
 * ========================================================================= */

/* =========================================================================
 * V-USB HARDWARE CONFIGURATION
 * D- = PIN 12 (PD3)
 * D+ = PIN 13 (PD4)
 * ========================================================================= */
#define USB_CFG_IOPORTNAME      D
#define USB_CFG_DMINUS_BIT      3
#define USB_CFG_DPLUS_BIT       4

/* V-USB uses D- (INT1) for SOF interrupts when USB_COUNT_SOF is 1. */
#define USB_INTR_CFG            MCUCR
#define USB_INTR_CFG_SET        ((1 << ISC11) | (0 << ISC10))
#define USB_INTR_CFG_CLR        0
#define USB_INTR_ENABLE         GICR
#define USB_INTR_ENABLE_BIT     INT1
#define USB_INTR_PENDING        GIFR
#define USB_INTR_PENDING_BIT    INTF1
#define USB_INTR_VECTOR         INT1_vect

/* USB power — stay within 100 mA USB spec (NFR-03) ----------------------- */
#define USB_MAX_POWER_CONSUMPTION 100

/* Debounce — sym_defer_g, 5 ms threshold --------------------------------- */
#define DEBOUNCE 5

/* Bootmagic Lite — hold (row 0, col 0) at power-on to enter bootloader
 * Disabled in P0; enabled in P8.                                          */
#define BOOTMAGIC_LITE_ROW    0
#define BOOTMAGIC_LITE_COLUMN 0

/* =========================================================================
 * NO SPLIT KEYBOARD
 *
 * This is a single-piece left-hand half keyboard.
 *   - No SPLIT_USB_DETECT
 *   - No SPLIT_HAND_PIN (PD2 freed for PS/2 CLK)
 *   - No SERIAL_USART_* (PD0/PD1 freed)
 *   - No SPLIT_TRANSACTION_IDS_USER
 * ========================================================================= */


/* ATmega32A Hardware Pins: XCK is PB0 (pin 40), RXD is PD0 (pin 9) */
#define PS2_CLOCK_PIN   B0
#define PS2_DATA_PIN    D0

// PS/2 Custom Vendor Driver configurations will be handled inside custom_ps2.c

/* PS/2 mouse sensitivity defaults (P2) */
#define PS2_MOUSE_X_MULTIPLIER 1
#define PS2_MOUSE_Y_MULTIPLIER 1
#define PS2_MOUSE_V_MULTIPLIER 1
// #define PS2_MOUSE_ENABLE_SCROLLING  /* DISABLED: see note below */
/* The IntelliMouse scroll-wheel unlock sequence (Set Sample Rate 200,100,80
 * + Get Device ID) requires ~200ms of blocking ps2_host_send() calls.
 * On V-USB this starves usbPoll(), AND if the sequence fails silently the
 * mouse stays in 3-byte mode while the parser expects 4 bytes, causing
 * permanent packet misalignment (phantom clicks + random scroll).
 * Re-enable only after implementing a fully non-blocking USART send. */
/* INIT_DELAY must be 0 for V-USB coexistence! The QMK default of 1000ms
 * blocks the main loop and starves usbPoll(), causing USB enumeration to
 * fail. The mouse POR+BAT delay is handled by a deferred re-init in
 * housekeeping_task_kb instead. */
#define PS2_MOUSE_INIT_DELAY   0

/* V-USB Multi-Endpoint Stability
 * 1ms polling overwhelms the ATmega32A when managing multiple endpoints (EP1, EP3).
 * Increasing it to 10ms (the Low-Speed USB standard) prevents EPROTO -71 errors. */
#define USB_POLLING_INTERVAL_MS 10

/* =========================================================================
 * RGB Matrix — SK6812MINI GRB, bitbang driver on PB4
 *
 * PB4 = pin 44. Cannot use hardware SPI driver (SPI MOSI is PB5/pin 1).
 * Using WS2812 bitbang driver instead.
 *
 * Enabled in P3.
 * ========================================================================= */
#define WS2812_DI_PIN                  B4   /* PB4 pin 44 — bitbang */
#define WS2812_BYTE_ORDER              WS2812_BYTE_ORDER_GRB
#define RGB_MATRIX_MAXIMUM_BRIGHTNESS  200  /* caps inrush current (NFR-03) */
#define DRIVER_LED_TOTAL               35   /* one LED per key */

// Enable RGB Matrix Animations (Demo Mode)
// #define ENABLE_RGB_MATRIX_ALPHAS_MODS
// #define ENABLE_RGB_MATRIX_GRADIENT_UP_DOWN
#define ENABLE_RGB_MATRIX_BREATHING
// #define ENABLE_RGB_MATRIX_BAND_VAL
// #define ENABLE_RGB_MATRIX_CYCLE_ALL
// #define ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
// #define ENABLE_RGB_MATRIX_CYCLE_UP_DOWN
#define ENABLE_RGB_MATRIX_RAINBOW_MOVING_CHEVRON
// #define ENABLE_RGB_MATRIX_CYCLE_PINWHEEL
// #define ENABLE_RGB_MATRIX_CYCLE_SPIRAL
// #define ENABLE_RGB_MATRIX_DUAL_BEACON
// #define ENABLE_RGB_MATRIX_RAINBOW_BEACON
// #define ENABLE_RGB_MATRIX_RAINDROPS
// #define ENABLE_RGB_MATRIX_JELLYBEAN_RAINDROPS
// #define ENABLE_RGB_MATRIX_PIXEL_RAIN
// #define ENABLE_RGB_MATRIX_PIXEL_FLOW

// Reactive Effects (Respond to typing)
// #define ENABLE_RGB_MATRIX_TYPING_HEATMAP
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
// #define ENABLE_RGB_MATRIX_SOLID_REACTIVE
// #define ENABLE_RGB_MATRIX_SPLASH
// #define ENABLE_RGB_MATRIX_SOLID_SPLASH
/* LDR ambient brightness — ADC channel 6 (PA6, pin 31)
 * Circuit: VCC → LDR (~20 kΩ nominal) → PA6 → 10 kΩ → GND
 * Enabled in P4.                                                          */
#define LDR_POLL_INTERVAL_MS     100   /* ADC sample interval              */
#define LDR_ADC_DARK              80   /* darkness threshold               */
#define LDR_ADC_BRIGHT           900   /* bright-light threshold           */
#define LDR_MIN_BRIGHTNESS        20   /* minimum brightness floor (%)     */
#define LDR_DEFAULT_ENABLED     true   /* on by default; toggled by LDR_TOG*/
#define LDR_CAL_STEP               4   /* ADC units per press              */
#define LDR_CAL_MAX              127   /* ± max calibration offset         */

/* I2C module API — hardware TWI @ 100 kHz
 * Enabled in P6.                                                          */
#define I2C_MODULE_TIMEOUT_MS    2

/* 24LC1025 external EEPROM on I2C bus
 * 128 KB total (2 × 64 KB blocks at addr 0x50 and 0x54)
 * Page write: 128 bytes max, 5 ms write cycle                            */
#define EEPROM_EXT_ADDR          0x50   /* block 0 base address            */
#define EEPROM_EXT_ADDR_B1       0x54   /* block 1 base address            */
#define EEPROM_EXT_PAGE_SIZE     128
#define EEPROM_EXT_WRITE_MS      5

/* Piezo driver — Timer2 CTC, OC2 = PD7, pin 16
 *
 * f_timer = F_CPU / PIEZO_PRESCALER = 16 000 000 / 64 = 250 000 Hz
 * f_out   = f_timer / (2 × (OCR2 + 1))
 *
 * NOTE: 440 Hz is unreachable with prescaler /64 at 16 MHz (OCR2 = 283 > 255).
 *       Lowest reachable note: ~490 Hz (OCR2 = 254).
 *
 * DANGER: DO NOT use Timer1 / OC1B (PD4 = V-USB D+).
 * Enabled in P5.                                                          */
#define PIEZO_PIN                D7
#define PIEZO_PRESCALER          64
#define PIEZO_NOTE_C5           239    /* 523 Hz (Do)                       */
#define PIEZO_NOTE_D5           212    /* 587 Hz (Re)                       */
#define PIEZO_NOTE_E5           189    /* 659 Hz (Mi)                       */
#define PIEZO_NOTE_F5           178    /* 698 Hz (Fa)                       */
#define PIEZO_NOTE_G5           158    /* 784 Hz (Sol)                      */
#define PIEZO_NOTE_A5           141    /* 880 Hz (La)                       */
#define PIEZO_NOTE_B5           126    /* 988 Hz (Si)                       */
#define PIEZO_CLICK_DURATION_MS   3    /* key-click pulse duration          */
#define PIEZO_LAYER_TONE_MS      30    /* per-note duration, layer effect   */

/* =========================================================================
 * OLED Configuration
 * ========================================================================= */
#define OLED_TIMEOUT 30000 // Turn off after 30 seconds of inactivity to prevent burn-in
#define OLED_BRIGHTNESS 128
#define OLED_DISPLAY_ADDRESS 0x3C
#define OLED_DISPLAY_128X64