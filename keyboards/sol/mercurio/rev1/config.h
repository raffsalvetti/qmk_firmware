#pragma once

/* =========================================================================
 * sol/mercurio rev1 — QMK firmware configuration
 * MCU : ATmega32A, 44-pin TQFP
 * USB : V-USB, 12 MHz external crystal (NFR-06)
 * FSD : rev 1.5
 *
 * USB identity (vid, pid, manufacturer, keyboard_name, device_version) is
 * defined in keyboard.json — do not duplicate here.
 * ========================================================================= */

/* USB power — stay within the 100 mA USB spec (NFR-03) ------------------- */
#define USB_MAX_POWER_CONSUMPTION 100

/* Debounce — sym_defer_g, 5 ms threshold (FR-02) ------------------------- */
#define DEBOUNCE 5

/* Bootmagic Lite — hold (row 0, col 0) at power-on to enter bootloader
 * (FR-15); same key on each half because both halves share the binary.    */
#define BOOTMAGIC_LITE_ROW    0
#define BOOTMAGIC_LITE_COLUMN 0

/* Split transport — dynamic master via USB detect (FR-04, FR-05)
 *
 * SPLIT_TRANSACTION_IDS_USER is commented out for P0/P1: it pulls in the
 * split-transaction enum which is not compiled until split_keyboard is
 * enabled in P2.  All other split defines are safe preprocessor values
 * that cause no harm when the split driver is absent.                     */
#define SPLIT_USB_DETECT
#define SPLIT_HAND_PIN          D2   /* PD2 pin 11: left=LOW (10 kΩ pull-down on left PCB),
                                                    right=HIGH (internal pull-up)          */
#define SERIAL_USART_SPEED      115200
#define SERIAL_USART_TX_PIN     D1   /* PD1 pin 10 */
#define SERIAL_USART_RX_PIN     D0   /* PD0 pin  9 */
#define SERIAL_USART_FULL_DUPLEX
/* #define SPLIT_TRANSACTION_IDS_USER MERCURIO_STATE_SYNC */   /* enable in P2 */

/* RGB Matrix — SK6812MINI GRB, hardware SPI MOSI on PB5 (FR-08)
 *
 * RGB_MATRIX_SPLIT is commented out: LEFT_LED_COUNT / RIGHT_LED_COUNT are
 * PCB-layout constants, undefined until PCB is final (P3).               */
#define WS2812_DI_PIN                  B5   /* PB5 pin 1 — SPI MOSI */
#define WS2812_BYTE_ORDER              WS2812_BYTE_ORDER_GRB
#define RGB_MATRIX_MAXIMUM_BRIGHTNESS  200  /* caps inrush current; do not exceed (NFR-03) */
/* #define RGB_MATRIX_SPLIT { LEFT_LED_COUNT, RIGHT_LED_COUNT } */  /* enable in P3 */

/* LDR ambient brightness — ADC channel 6 (PA6, pin 31) (FR-09)
 * Circuit: VCC → LDR (~20 kΩ nominal) → PA6 → 10 kΩ → GND              */
#define LDR_POLL_INTERVAL_MS     100   /* ADC sample interval              */
#define LDR_ADC_DARK              80   /* darkness threshold (LDR >> 10 kΩ)*/
#define LDR_ADC_BRIGHT           900   /* bright-light threshold           */
#define LDR_MIN_BRIGHTNESS        20   /* minimum brightness floor (%)     */
#define LDR_DEFAULT_ENABLED     true   /* on by default; toggled by LDR_TOG*/
#define LDR_CAL_STEP               4   /* ADC units per LDR_CAL_UP/DN press*/
#define LDR_CAL_MAX              127   /* ± max calibration offset         */

/* I2C module API — hardware TWI @ 100 kHz (FR-10) ----------------------- */
#define I2C_MODULE_TIMEOUT_MS    2
#define OLED_MODULE_ADDR      0x20    /* ATtiny48 + SSD1306, left half     */
#define MOUSE_MODULE_ADDR     0x21    /* ATtiny45 + PSP joystick, right    */

/* Piezo driver — Timer2 CTC, OC2 = PD7, pin 16 (FR-13, NFR-07, NFR-08)
 *
 * f_timer = F_CPU / PIEZO_PRESCALER = 12 000 000 / 64 = 187 500 Hz
 * f_out   = f_timer / (2 × (OCR2 + 1))
 *
 * DANGER: DO NOT use Timer1 / OC1B (PD4 = V-USB D+).                    */
#define PIEZO_PIN                D7
#define PIEZO_PRESCALER          64
#define PIEZO_NOTE_440HZ        212    /* A4  ~440 Hz                       */
#define PIEZO_NOTE_1KHZ          92    /* ~1010 Hz  (left-half key click)   */
#define PIEZO_NOTE_2KHZ          46    /* ~1976 Hz  (right-half key click)  */
#define PIEZO_NOTE_4KHZ          22    /* ~3978 Hz                          */
#define PIEZO_CLICK_DURATION_MS   3    /* key-click pulse duration          */
#define PIEZO_LAYER_TONE_MS      30    /* per-note duration, layer effect   */
