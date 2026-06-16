# Mercurio rev1 — Firmware Development Plan

## sol/mercurio rev1 · ATmega32A · QMK

| Field    | Value                        |
| :------- | :--------------------------- |
| MCU      | ATmega32A, 12 MHz crystal    |
| Firmware | QMK                          |
| FSD Rev  | 1.5                          |
| Status   | Draft                        |

---

## Overview

This document translates the Functional Specification (FSD rev 1.5) into an ordered, incremental development plan. Each phase has a concrete goal, a list of files to produce or modify, key implementation notes, and a set of gate criteria that must pass before proceeding to the next phase.

**Guiding principles:**
- Build the narrowest possible vertical slice first (Phase 1 = USB + keypress, nothing else).
- Every phase ends in a testable, flashing firmware — no dead-end stubs.
- V-USB timing constraints apply from Phase 1 onward; no driver may hold the CPU for more than ~50 µs.
- Flash budget is monitored continuously; target headroom of ≥2 KB at every phase boundary.

**Dependency graph:**

```
P0 (skeleton)
 └── P1 (USB + matrix)
      └── P2 (split transport)
           ├── P3 (RGB matrix)
           │    └── P4 (LDR + calibration)
           ├── P5 (piezo)
           └── P6 (I2C module API)
                ├── P7 (OLED module)      ← also depends on P3, P4, P5
                └── P8 (mouse module)
      └── P9 (keymap + ADJUST layer)      ← integrates all features
           └── P10 (bootloader + final validation)
```

---

## Phase 0 — Toolchain & Skeleton

**Goal:** The project compiles cleanly for ATmega32A and produces a valid (empty) firmware binary. No functionality yet.

### Files to create

| File | Purpose |
| :--- | :------ |
| `keyboards/sol/mercurio/rev1/keyboard.json` | MCU, bootloader, USB IDs, matrix pins, feature flags |
| `keyboards/sol/mercurio/rev1/rules.mk` | Feature enables/disables, `SRC` list (stubs only) |
| `keyboards/sol/mercurio/rev1/config.h` | All `#define` constants from FSD §A |
| `keyboards/sol/mercurio/rev1/mercurio.h` | `mercurio_split_state_t`, extern, transaction ID |
| `keyboards/sol/mercurio/rev1/mercurio.c` | Empty `keyboard_post_init_kb()`, `housekeeping_task_kb()` stubs |
| `keyboards/sol/mercurio/rev1/keymaps/default/keymap.c` | Single BASE layer, all keys as `KC_TRNS` except `QK_BOOT` on (0,0) |

### Implementation notes

**`keyboard.json`** — disable everything not needed yet:
```json
"features": {
    "bootmagic": false,
    "mousekey":  false,
    "extrakey":  false,
    "rgb_matrix": false,
    "pointing_device": false,
    "split_keyboard": false
}
```
Enable features incrementally as each phase lands to keep the binary small and isolate build failures.

**`rules.mk`** — start minimal:
```makefile
MCU        = atmega32a
F_CPU      = 12000000
BOOTLOADER = usbasploader
AUDIO_ENABLE = no
CONSOLE_ENABLE = no
COMMAND_ENABLE = no
BLUETOOTH_ENABLE = no
```

**`config.h`** — populate all constants from FSD §A now (pins, ADC thresholds, piezo notes, I2C addresses, calibration limits) even for features not yet implemented. Centralising them here avoids scatter later.

**`mercurio_split_state_t`** — define the full struct from FSD §5.1 now. It will grow only by adding fields; existing fields must never shift (USART framing).

### Gate criteria

- [ ] `qmk compile -kb sol/mercurio/rev1 -km default` succeeds with zero errors and zero warnings.
- [ ] `.build/*.map` confirms target MCU is `atmega32a`, flash usage < 5 KB.
- [ ] No `#warning` or `#error` from V-USB timing headers (confirms 12 MHz is accepted).

---

## Phase 1 — USB Enumeration + Matrix (Single Half)

**Goal:** One half enumerates as a USB HID keyboard. All 35 local keys register correct keycodes on the host. Debounce works. No split yet.

### Files to create / modify

| File | Action |
| :--- | :----- |
| `keyboard.json` | Enable `mousekey`, `extrakey`; set `diode_direction`, `matrix_pins` |
| `config.h` | `DEBOUNCE 5`, `USB_MAX_POWER_CONSUMPTION 100` |
| `keymaps/default/keymap.c` | Populate BASE layer with real keycodes (Option A from FSD §E) |

### Implementation notes

**V-USB crystal requirement:** The ATmega32A ships with the internal RC oscillator selected. The firmware will not enumerate until the fuses are programmed for the external crystal. This is a one-time hardware step, not a firmware step, but it blocks all USB testing:

```bash
avrdude -p m32 -c usbasp \
  -U lfuse:w:0xFF:m \
  -U hfuse:w:0xD9:m
```

`0xFF` = external full-swing crystal, no clock divide. `0xD9` = SPI enabled, no JTAG, no WDT, BOD at 4.0 V. Verify against ATmega32A datasheet Table 2 before writing.

**Matrix wiring:** COL2ROW. Columns (outputs) are driven LOW one at a time; rows (inputs) have internal pull-ups enabled. A pressed key pulls the row LOW through the diode (anode at row, cathode at column). Confirm `DIODE_DIRECTION = COL2ROW` in `keyboard.json`.

**Row/col pin order must match PCB layout exactly:**
```c
MATRIX_ROW_PINS = { A0, A1, A2, A3, A4 }   // PA0–PA4
MATRIX_COL_PINS = { C2, C3, C4, C5, C6, C7, A7 }
```
Any transposition here causes the wrong key to register. Map against the PCB schematic before first flash.

**USB HID composite device:** `MOUSE_SHARED_EP = no` must be set before enabling pointing device in a later phase. Add it to `config.h` now to avoid a later breaking change.

**Debounce:** `sym_defer_g` is the QMK default for split keyboards. Set `DEBOUNCE 5` in `config.h`. Do not use `sym_eager_pk` — it adds per-key state which is expensive in SRAM on a 2 KB budget.

**SRAM budget awareness:** ATmega32A has 2 KB SRAM. QMK's matrix, debounce, and USB descriptors consume roughly 600–800 bytes. Monitor `avr-size` output at every phase. If SRAM usage approaches 85% (1.7 KB), start pruning.

### Gate criteria

- [ ] Host enumerates the device as a USB HID keyboard (confirm with `lsusb` / Device Manager).
- [ ] All 35 keys on one half register their correct keycodes in a key-tester application.
- [ ] No keycode fires without a physical press (no ghost, no stuck key).
- [ ] Holding a key for 1 second produces no repeat jitter in the host event log.
- [ ] Flash usage < 18 KB (leaves ≥14 KB headroom for remaining features).

---

## Phase 2 — Split Transport

**Goal:** Both halves connected via TRRS. Master/Slave is assigned dynamically. Slave key presses register on the host as the correct unified matrix positions.

### Files to create / modify

| File | Action |
| :--- | :----- |
| `keyboard.json` | Enable `split_keyboard: true`; set `serial.driver: usart`, `handedness.pin: D2` |
| `config.h` | `SPLIT_USB_DETECT`, `SERIAL_USART_SPEED 115200`, TX/RX pins, `SERIAL_USART_FULL_DUPLEX` |
| `mercurio.h` | Add `SPLIT_TRANSACTION_IDS_USER MERCURIO_STATE_SYNC` |
| `mercurio.c` | Register split transaction: `transaction_register_rpc()`, sync callback stubs |

### Implementation notes

**`SPLIT_USB_DETECT`:** QMK probes D+ at boot. The half whose D+ pull-up responds becomes Master; the other becomes Slave. This requires no `MASTER_RIGHT` or `MASTER_LEFT` define. Both halves run identical firmware.

**Handedness pin:** PD2 (pin 11). Left PCB has a 10 kΩ resistor to GND (reads LOW = left). Right PCB relies on the internal pull-up (reads HIGH = right). QMK default: LOW = left. Confirm `SPLIT_HAND_PIN D2` in `config.h`. EEPROM handedness storage activates automatically on first valid read.

**USART wiring over TRRS:**
```
Tip    → VCC
Ring 1 → GND
Ring 2 → TX of one half → RX of other (and vice versa)
Sleeve → RX of first  → TX of second
```
Full duplex requires `SERIAL_USART_FULL_DUPLEX`. PD0 = RX, PD1 = TX on both halves.

**Unified matrix row offset:** Left half owns rows 0–4, right half rows 5–9 in the unified 10-row matrix. This is automatic with QMK split when `MATRIX_ROWS = 10` and `SPLIT_USB_DETECT` is used. The keymap's right-half keys must be placed on rows 5–9.

**Split state sync — initial version:** For this phase, `mercurio_split_state_t` only needs `active_layer`. Keep the sync payload minimal until later phases add fields. The struct is already defined with all fields from Phase 0; just don't write to unused fields yet.

**TRRS hot-plug:** Plugging TRRS while USB is live can cause a USART framing error. QMK's USART driver auto-resynchronises but may drop a few scan cycles. Advise users to plug TRRS before USB. This is a known, accepted risk (FSD §11).

### Gate criteria

- [ ] Plugging USB into the left half: left = Master, right = Slave.
- [ ] Plugging USB into the right half: right = Master, left = Slave.
- [ ] All 35 keys on the Slave half register correctly on the host (rows 5–9 in unified matrix).
- [ ] No split key misfires when only one half is connected (Master-only mode still works).
- [ ] `active_layer` field syncs from Master to Slave (verify via debug output or LED pattern).

---

## Phase 3 — RGB Matrix

**Goal:** All SK6812MINI LEDs on both halves illuminate and respond to RGB Matrix animations and manual controls.

### Files to create / modify

| File | Action |
| :--- | :----- |
| `keyboard.json` | Enable `rgb_matrix: true`; set `ws2812.pin: B5`, `ws2812.driver: spi`, `byte_order: GRB` |
| `config.h` | `RGB_MATRIX_MAXIMUM_BRIGHTNESS 200`, `RGB_MATRIX_SPLIT {LEFT_LED_COUNT, RIGHT_LED_COUNT}` |
| `mercurio.h` or `config.h` | Define `LEFT_LED_COUNT`, `RIGHT_LED_COUNT` once PCB layout is final |

### Implementation notes

**SPI driver for WS2812:** The ATmega32A's hardware SPI generates the WS2812 bit-stream at 12 MHz without CPU stall. MOSI = PB5 (pin 1). SCK = PB7 (pin 2) is driven but not wired to the LED chain; only MOSI matters for WS2812. Confirm `WS2812_DI_PIN B5` in `config.h`.

**`RGB_MATRIX_SPLIT`:** This macro tells QMK how many LEDs are on each half so it can split the frame buffer correctly over USART. It must match the physical LED count exactly. If the count is wrong, LEDs on the Slave half will show garbled colours. Leave `LEFT_LED_COUNT` and `RIGHT_LED_COUNT` as PCB-defined constants until the layout is final — do not hardcode integers.

**LED position map:** QMK RGB Matrix requires a `g_led_config` struct in `mercurio.c` that maps each LED index to a physical (x, y) position and flags it as per-key or underglow. This table is layout-dependent; populate it once the PCB footprint is confirmed. For initial testing, all LEDs can be flagged `LED_FLAG_ALL` with evenly-spaced dummy coordinates.

**`RGB_MATRIX_MAXIMUM_BRIGHTNESS 200`:** Caps the hardware PWM duty cycle to prevent inrush current from exceeding the USB 100 mA budget when all LEDs are white. Do not raise this above 200 without re-measuring current draw.

**Animations to enable initially:** Start with `RGB_MATRIX_SOLID_COLOR` only. Add `RGB_MATRIX_BREATHING` and `RGB_MATRIX_CYCLE_ALL` once current budget is confirmed. Each additional animation adds ~200–500 bytes of flash.

### Gate criteria

- [ ] All LEDs on both halves illuminate in the same solid colour.
- [ ] `RGB_TOG` (temporary ADJUST binding) turns all LEDs off and on.
- [ ] `RGB_MOD` cycles through at least two animations.
- [ ] No LED on the Slave half flickers or shows incorrect colour relative to the Master.
- [ ] Current draw with all LEDs white at maximum brightness measured ≤ 100 mA (USB spec).

---

## Phase 4 — LDR Ambient Dimming + Toggle + Calibration

**Goal:** Each half automatically dims its LEDs based on ambient light. The user can toggle auto-dimming on/off and calibrate each half's LDR offset independently. Both states persist across power cycles.

### Files to create / modify

| File | Action |
| :--- | :----- |
| `ldr.c` | `ldr_init()`, `ldr_task()`, `ldr_toggle()`, `ldr_cal_adjust()`, EEPROM helpers |
| `ldr.h` | Public API declarations, EEPROM address defines |
| `mercurio.c` | Call `ldr_init()` from `keyboard_post_init_kb()`; call `ldr_task()` from `housekeeping_task_kb()` |
| `mercurio.h` | Ensure `ldr_enabled`, `ldr_cal_offset` are present in `mercurio_split_state_t` |
| `rules.mk` | `SRC += ldr.c` |

### Implementation notes

**ADC setup:** ATmega32A's ADC requires explicit initialisation (`ADMUX`, `ADCSRA`). QMK does not initialise the ADC for you on non-ARM targets. In `ldr_init()`:
```c
ADMUX  = (1 << REFS0);          // AVcc reference
ADCSRA = (1 << ADEN) | 0x07;   // Enable ADC, prescaler /128 → 93.75 kHz @ 12 MHz
```
ADC channel 6 (PA6) is selected at conversion time by writing `ADMUX = (1 << REFS0) | 6`.

**Non-blocking conversion:** Start a conversion in one `ldr_task()` call; read the result in the next (check `ADCSRA & (1 << ADIF)`). Never busy-wait for `ADIF` — that could stall for up to ~110 µs and violate V-USB's ≤50 µs rule.

**Normalization + hysteresis:**
```c
int16_t adc_cal  = (int16_t)adc_raw + ldr_cal_local;
adc_cal          = clamp(adc_cal, 0, 1023);
uint8_t target   = map(adc_cal, LDR_ADC_DARK, LDR_ADC_BRIGHT,
                        LDR_MIN_BRIGHTNESS, 100);
// 5-step hysteresis: only update if |current - target| > 5
if (abs((int8_t)(target - current_brightness)) > 5) {
    current_brightness = target;
    rgb_matrix_set_val(target * RGB_MATRIX_MAXIMUM_BRIGHTNESS / 100);
    mercurio_state.led_brightness = target;
}
```

**EEPROM layout for LDR state:** QMK reserves the first ~20 bytes of EEPROM for its own config (magic, debug, audio, rgb, etc.). Use `EECONFIG_USER` (a user-defined block) or a fixed address beyond QMK's range. A safe starting offset is `0x20`. Store:
```
0x20 : uint8_t  ldr_enabled   (0 = off, 1 = on)
0x21 : int8_t   ldr_cal_local (signed, ±127)
```
On first boot (magic bytes absent), initialise to `ldr_enabled = LDR_DEFAULT_ENABLED`, `ldr_cal_local = 0`.

**Per-half calibration flow:**

- `LDR_CAL_UP` / `LDR_CAL_DN` are custom keycodes (define in an enum in `mercurio.h` starting above `SAFE_RANGE`).
- In `process_record_kb()`, determine the physical side of the pressed key:
  ```c
  bool key_is_left = (record->event.key.row < MATRIX_ROWS / 2);
  bool i_am_left   = is_keyboard_left();
  bool is_local    = (key_is_left == i_am_left);
  ```
- If local: adjust `ldr_cal_local`, write EEPROM.
- If remote (Slave's key pressed, handled on Master): adjust `mercurio_state.ldr_cal_offset`, which propagates to Slave on next sync. In `housekeeping_task_slave()`, detect change and save to local EEPROM.

**LDR_TOG EEPROM write:** Write on every toggle — the operation is infrequent enough that EEPROM wear is not a concern (rated 100 000 cycles on ATmega32A).

### Gate criteria

- [ ] In a dark room, LED brightness decreases to ≥ `LDR_MIN_BRIGHTNESS` (20%) within 200 ms.
- [ ] In bright ambient light, brightness reaches close to 100%.
- [ ] No rapid flickering when light is at a threshold boundary (hysteresis confirmed).
- [ ] `LDR_TOG` disables auto-dimming; brightness no longer tracks the sensor.
- [ ] After power-cycle, LDR toggle state is restored from EEPROM.
- [ ] `LDR_CAL_UP` ×3 on the left half: left LED response curve shifts; right half unaffected.
- [ ] `LDR_CAL_UP` ×3 on the right half: right LED response curve shifts; left half unaffected.
- [ ] Calibration offset persists after power-cycle on both halves.
- [ ] `EE_CLR` + power-cycle resets both offsets to 0 and re-enables auto-dimming.

---

## Phase 5 — Piezo Audio Driver

**Goal:** Both halves play audible tones via their piezo transducers. Key clicks fire on key-down. Layer-change and startup effects work. Dual-channel synchronisation between halves is within ~1 ms.

### Files to create / modify

| File | Action |
| :--- | :----- |
| `piezo.c` | `piezo_init()`, `piezo_play()`, `piezo_stop()`, `piezo_task()` |
| `piezo.h` | Public API, OCR2 note constants |
| `piezo_effects.c` | `piezo_effect_keyclick()`, `piezo_effect_layer_change()`, `piezo_effect_startup()` |
| `piezo_effects.h` | Effect declarations |
| `mercurio.c` | Call `piezo_init()` from `keyboard_post_init_kb()`; call `piezo_task()` from `housekeeping_task_kb()`; call startup effect; sync slave in `housekeeping_task_kb()` |
| `mercurio.h` | Ensure `piezo_ocr2`, `piezo_duration_ms` are in `mercurio_split_state_t` |
| `rules.mk` | `SRC += piezo.c piezo_effects.c` |

### Implementation notes

**Timer2 CTC setup in `piezo_init()`:**
```c
TCCR2 = 0;                              // stop timer, clear config
DDRB  |= (1 << PD7);                   // Wait — OC2 is PD7; set as output
// Actually: DDRD |= (1 << PD7);
OCR2  = 0;
TCCR2 = (1 << WGM21)                   // CTC mode
      | (1 << COM20)                    // Toggle OC2 on compare match (hardware)
      | (1 << CS22);                    // Prescaler /64 → f_timer = 187.5 kHz
```
`CS22 = 1, CS21 = 0, CS20 = 0` = prescaler /64 on ATmega32A Timer2. Confirm against ATmega32A datasheet Table 55 — the prescaler bits differ from ATmega32U4.

**OC2 = PD7, pin 16.** Set `DDRD |= (1 << PD7)` in `piezo_init()`. The hardware compare match toggles PD7 with zero CPU involvement during tone playback.

**Timer conflict avoidance:** Timer1 is used by V-USB (OC1B = PD4 = D+). Timer0 drives QMK's millisecond tick (`timer_read32()`). Timer2 is the only safe choice for audio. Do not enable any QMK audio subsystem that touches Timer1 or Timer3.

**`piezo_play()`:**
```c
void piezo_play(uint8_t ocr2, uint8_t duration_ms) {
    OCR2             = ocr2;
    piezo_duration   = duration_ms;    // decremented in piezo_task()
    TCCR2           |= (1 << COM20);   // enable OC2 toggle
}
```

**`piezo_task()` (called every 1 ms):**
```c
void piezo_task(void) {
    if (piezo_duration == 0) return;
    if (--piezo_duration == 0) piezo_stop();
}
```
This is ~8 cycles per scan, well within the V-USB ≤50 µs budget.

**`piezo_stop()`:**
```c
void piezo_stop(void) {
    TCCR2  &= ~(1 << COM20);     // disable OC2 toggle
    PORTD  &= ~(1 << PD7);       // drive pin low (silence)
    piezo_duration = 0;
}
```

**Dual-channel sync:** In `process_record_kb()` on key-down, `piezo_effect_keyclick()` fires the local (Master) note and writes `mercurio_state.piezo_ocr2` / `piezo_duration_ms` for the Slave. In `housekeeping_task_kb()` on the Slave side, check if `piezo_ocr2 > 0` and call `piezo_play()`, then clear the field to avoid replaying.

**OCR2 values from FSD §5.5:**
```c
#define PIEZO_NOTE_1KHZ   92    // ~1010 Hz actual
#define PIEZO_NOTE_2KHZ   46    // ~1976 Hz actual
```
Left half (Slave in most setups): 1 kHz. Right half (Master when right USB is used, or Slave when left USB is used): use `is_keyboard_left()` to assign note pitch per half.

### Gate criteria

- [ ] Pressing any key on either half produces an audible click.
- [ ] Left half click is audibly lower pitch than right half click (~1 kHz vs ~2 kHz).
- [ ] Both halves click within the same perceptible instant (≤1 ms sync latency is inaudible).
- [ ] Layer change produces a two-tone ascending effect on both halves.
- [ ] Power-on startup sweep plays on both halves.
- [ ] Holding a key down does not produce a continuous tone (one click per press, then silence).
- [ ] Tone stops cleanly within 3 ms (`PIEZO_CLICK_DURATION_MS`).

---

## Phase 6 — I2C Module API

**Goal:** A generic TWI master API is operational. It can write to and read from any TWI slave device on the local bus, with a 2 ms timeout. Both halves' local buses work independently.

### Files to create / modify

| File | Action |
| :--- | :----- |
| `i2c_module.c` | `i2c_module_init()`, `i2c_module_write()`, `i2c_module_read()` |
| `i2c_module.h` | API declarations, `module_status_t` enum, timeout constant |
| `mercurio.c` | Call `i2c_module_init()` for both slots from `keyboard_post_init_kb()` |
| `rules.mk` | `SRC += i2c_module.c`; ensure QMK's `I2C_DRIVER = i2c` or use bare TWI |

### Implementation notes

**QMK's TWI driver vs. bare AVR TWI:** QMK provides `i2c_master.h` (`i2c_init`, `i2c_write_register`, `i2c_read_register`) built on the ATmega hardware TWI peripheral. Use QMK's driver as the transport layer inside `i2c_module.c` rather than writing raw TWI sequences. This keeps the code portable and lets QMK handle the TWI interrupt flag management.

**Timeout enforcement:** QMK's `i2c_write_register` / `i2c_read_register` accept a `timeout_ms` parameter. Pass `I2C_MODULE_TIMEOUT_MS` (2) for every call. Never call I2C from an ISR context.

**Call site restriction:** All `i2c_module_write` / `i2c_module_read` calls must originate from `housekeeping_task_kb()` — never from `matrix_scan()`, never from a keycode handler, never from an ISR. This is the only hook that runs outside the critical V-USB interrupt path with enough margin.

**`module_status_t` return values:** Callers must check the return. A `MODULE_TIMEOUT` from the mouse module must zero the mouse report; a `MODULE_TIMEOUT` from the OLED module is silently ignored (stale display is acceptable).

**Initial test without ATtiny modules:** During Phase 6, test the API by scanning for any I2C device on the bus using `i2c_module_init(0x20, ...)` and checking for a non-NACK response. A logic analyser on SCL/SDA (PC0/PC1) confirms the bus waveform at 100 kHz.

### Gate criteria

- [ ] `i2c_module_init()` returns `MODULE_OK` when an ATtiny module is present at the expected address.
- [ ] `i2c_module_init()` returns `MODULE_TIMEOUT` (not a hang) when no device is present.
- [ ] A 1-byte write + 1-byte read round-trip completes within 2 ms (measured with logic analyser or timer).
- [ ] Calling the API from `housekeeping_task_kb()` does not cause USB HID report drops.
- [ ] Left and right half buses operate independently (a missing module on one half does not affect the other).

---

## Phase 7 — OLED Module (Left Half)

**Goal:** The SSD1306 OLED on the left half displays: active layer, Caps Lock state, LED brightness percentage, audio volume, LDR on/off indicator, and LDR calibration offset. Updates within 50 ms of any state change.

### Files to create / modify

| File | Action |
| :--- | :----- |
| `oled_module.c` | `oled_module_init()`, `oled_module_task()` |
| `oled_module.h` | Public API declarations |
| `mercurio.c` | Call `oled_module_init()` from `keyboard_post_init_kb()` (left half only); call `oled_module_task()` from `housekeeping_task_kb()` |
| `rules.mk` | `SRC += oled_module.c` |

### Implementation notes

**Left-half-only execution:** Gate all OLED calls behind `is_keyboard_left()`:
```c
void oled_module_task(void) {
    if (!is_keyboard_left()) return;
    if (!timer_elapsed32(oled_timer) > OLED_POLL_INTERVAL_MS) return;
    oled_timer = timer_read32();
    // ... build and send state
}
```

**`SET_STATE_BULK` (register `0x14`, 5 bytes):** Send the bulk update on every `oled_module_task()` cycle. The ATtiny48 handles display rendering; QMK only pushes data. Payload: `[active_layer, caps_lock, led_brightness, audio_volume, flags]` where flags bit 0 = `ldr_enabled`.

**`SET_LDR_CAL` (register `0x17`, 1 byte, signed):** Send only when the local LDR calibration offset changes. Track last-sent value in a static variable; compare before writing.

**State sources on the left half:**
- `mercurio_state.active_layer` — synced from Master via USART (always current)
- `host_keyboard_led_state().caps_lock` — call on Master only; propagated via `mercurio_state.caps_lock`
- `mercurio_state.led_brightness` — updated by `ldr_task()`
- `mercurio_state.ldr_enabled` — updated by `ldr_toggle()`
- `ldr_cal_local` — local variable in `ldr.c`; accessible via `ldr_get_cal_offset()`

**TWI bus occupancy:** A `SET_STATE_BULK` write (5 bytes + address + register = 7 bytes on the wire at 100 kHz) takes approximately 630 µs. This is within the 2 ms timeout and well within the 50 ms poll interval.

### Gate criteria

- [ ] OLED displays layer index; changes within 50 ms of a layer key press.
- [ ] OLED displays Caps Lock state; updates on Caps Lock toggle.
- [ ] OLED displays LED brightness percentage; updates as LDR varies.
- [ ] OLED shows `LDR ON` / `LDR OFF` and toggles within 50 ms of `LDR_TOG`.
- [ ] OLED shows `CAL:+N` and updates within 50 ms of `LDR_CAL_UP` / `LDR_CAL_DN`.
- [ ] A missing or powered-off ATtiny48 causes no crash and no USB stall (timeout gracefully ignored).

---

## Phase 8 — Mouse Module (Right Half)

**Goal:** The PSP analog thumbstick controls the host cursor. All three physical mouse buttons (left click, right click, stick click) function. Deadzone and sensitivity are configurable.

### Files to create / modify

| File | Action |
| :--- | :----- |
| `mouse_module.c` | `mouse_module_init()`, `mouse_module_task()` |
| `mouse_module.h` | Public API declarations |
| `mercurio.c` | Call `mouse_module_init()` from `keyboard_post_init_kb()` (right half only); call `mouse_module_task()` from `housekeeping_task_kb()` |
| `config.h` | `POINTING_DEVICE_ENABLE = yes` is already in `keyboard.json`; confirm `POINTING_DEVICE_DRIVER = custom` |
| `rules.mk` | `SRC += mouse_module.c` |

### Implementation notes

**Right-half-only execution:** Gate behind `!is_keyboard_left()`:
```c
void mouse_module_task(void) {
    if (is_keyboard_left()) return;
    if (!timer_elapsed32(mouse_timer) > MOUSE_POLL_INTERVAL_MS) return;
    mouse_timer = timer_read32();
    // ...
}
```

**`GET_AXES` read (register `0x10`, 4 bytes):** Returns `[int8_t dx, int8_t dy, uint8_t buttons, uint8_t status]`. Check `status & 0x01` (ready bit) before injecting a report. If not ready or timeout: inject a zeroed report (no phantom motion).

**Pointing device report injection:**
```c
report_mouse_t report = {};
report.x       = axes[0];    // int8_t dx
report.y       = axes[1];    // int8_t dy
report.buttons = axes[2];    // button bitmask
pointing_device_set_report(report);
pointing_device_send();
```

**Button bitmask (from FSD §5.4):**
```
Bit 0 → stick click → MOUSE_BTN3
Bit 1 → left click  → MOUSE_BTN1
Bit 2 → right click → MOUSE_BTN2
```
Map these bits to QMK's `MOUSE_BTN` macros before assigning to `report.buttons`.

**`MOUSE_SHARED_EP = no`:** This was set in `config.h` during Phase 1. Confirm it is present. Without it, the mouse shares an endpoint with the keyboard, causing dropped reports at high polling rates.

**Deadzone:** The ATtiny45 applies its own deadzone internally (configurable via `SET_DEADZONE` register). QMK-side deadzone is not needed. If the cursor drifts at rest, increase the ATtiny45's deadzone via `i2c_module_write()` at init time.

### Gate criteria

- [ ] Moving the thumbstick in all four cardinal directions moves the host cursor accordingly.
- [ ] Stick released to centre stops cursor movement (deadzone functional).
- [ ] All three buttons (left, right, stick-click) register distinct mouse button events on the host.
- [ ] A missing or powered-off ATtiny45 causes no crash, no phantom cursor movement, and no USB stall.
- [ ] Mouse reports do not cause keyboard HID report drops (separate endpoint confirmed).

---

## Phase 9 — Full Keymap + ADJUST Layer

**Goal:** All four layers are fully populated. Every ADJUST layer control key performs its specified firmware action. Custom keycodes for LDR toggle and calibration are wired up.

### Files to create / modify

| File | Action |
| :--- | :----- |
| `mercurio.h` | Define custom keycode enum: `LDR_TOG`, `LDR_CAL_UP`, `LDR_CAL_DN` starting at `SAFE_RANGE` |
| `keymaps/default/keymap.c` | Populate all four layers (BASE, LOWER, RAISE, ADJUST) per FSD §E |
| `mercurio.c` | Implement `process_record_kb()`: handle `LDR_TOG`, `LDR_CAL_UP`, `LDR_CAL_DN`, `piezo_effect_keyclick()` on any key-down |
| `mercurio.c` | Implement `layer_state_set_kb()`: call `piezo_effect_layer_change()` |

### Implementation notes

**Custom keycode enum:**
```c
enum mercurio_keycodes {
    LDR_TOG    = SAFE_RANGE,
    LDR_CAL_UP,
    LDR_CAL_DN,
};
```
`SAFE_RANGE` ensures no collision with QMK's built-in keycodes.

**`process_record_kb()` structure:**
```c
bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        switch (keycode) {
            case LDR_TOG:    ldr_toggle(); return false;
            case LDR_CAL_UP: ldr_cal_adjust(+LDR_CAL_STEP); return false;
            case LDR_CAL_DN: ldr_cal_adjust(-LDR_CAL_STEP); return false;
            default:
                piezo_effect_keyclick();
                break;
        }
    }
    return process_record_user(keycode, record);
}
```
Return `false` for custom keycodes to prevent them from being forwarded to the host as HID keycodes.

**ADJUST layer placement:** ADJUST is accessed by holding both LOWER and RAISE simultaneously. In the keymap, place `QK_BOOT` in a position that cannot be triggered accidentally (e.g., inner column, not on a thumb key). Place `EE_CLR` adjacent to `QK_BOOT` so the user is aware of its destructive effect.

**`EE_CLR` side effects:** This resets all QMK EEPROM, including RGB settings, stored handedness, and the user EEPROM block (`ldr_enabled`, `ldr_cal_local`). After `EE_CLR` + power-cycle, both LDR defaults must be restored from firmware constants (`LDR_DEFAULT_ENABLED`, offset = 0). Verify this in TC-16.

**`layer_state_set_kb()`:**
```c
layer_state_t layer_state_set_kb(layer_state_t state) {
    uint8_t layer = get_highest_layer(state);
    mercurio_state.active_layer = layer;
    piezo_effect_layer_change(layer);
    return layer_state_set_user(state);
}
```

### Gate criteria

- [ ] BASE layer: all alphanumeric, modifier, and symbol keys register correctly.
- [ ] LOWER layer: symbol overrides register; unassigned positions pass through as `KC_TRNS`.
- [ ] RAISE layer: navigation and F-key overrides register; mouse keys functional.
- [ ] ADJUST layer: `RGB_TOG`, `RGB_MOD`, `LDR_TOG`, `LDR_CAL_UP`, `LDR_CAL_DN`, `EE_CLR`, `QK_BOOT` all perform their specified actions.
- [ ] Pressing a key fires exactly one piezo click per physical press.
- [ ] Layer change fires the ascending two-tone effect.
- [ ] Custom keycodes do not appear as stray HID events on the host.

---

## Phase 10 — Bootloader, Final Validation & Release

**Goal:** USBasp-loader is installed. Bootmagic and `QK_BOOT` entry work on both halves. Firmware is within budget. All FSD test cases pass.

### Steps

**10a — Bootloader installation (ISP required, one-time per half):**
```bash
avrdude -p m32 -c usbasp \
  -U flash:w:usbasploader_m32a.hex:i
```
After this, use `avrdude` via USB for all subsequent flashes.

**10b — Enable Bootmagic in `keyboard.json`:**
```json
"bootmagic": true
```
Set `BOOTMAGIC_LITE_ROW 0`, `BOOTMAGIC_LITE_COLUMN 0` in `config.h`. Test: hold row 0, col 0 key at power-on → half enumerates as USBasp device.

**10c — Flash budget check:**
```bash
avr-size --format=avr --mcu=atmega32a \
  .build/sol_mercurio_rev1_default.elf
```
Target: flash ≤ 30 KB (≤94% of 32 KB). If over, apply the following in order:
1. Audit `rules.mk` for any accidentally enabled features.
2. Remove unused RGB Matrix animations (`#undef RGB_MATRIX_*`).
3. Replace `printf`-style debug strings with single-byte codes.
4. Profile with `avr-nm --size-sort` to find the largest symbols.

**10d — SRAM budget check:**
```
.data + .bss ≤ 1700 bytes (≤83% of 2 KB)
```
Stack grows downward from the top of SRAM. Leave ≥300 bytes for stack. If over, reduce `mercurio_split_state_t` padding, reduce RGB Matrix framebuffer depth, or reduce debounce history.

**10e — Full FSD test case execution:**

Run every test case from FSD §9 in order:

| Test | Phase verified |
| :--- | :------------- |
| TC-01 Matrix scan | P1 |
| TC-02 Debounce | P1 |
| TC-03 USART split | P2 |
| TC-04 Dynamic Master/Slave | P2 |
| TC-05 Handedness | P2 |
| TC-06 Shared state propagation | P2 |
| TC-07 USB composite HID | P1, P8 |
| TC-08 RGB Matrix | P3 |
| TC-09 LDR dimming | P4 |
| TC-09b LDR toggle | P4 |
| TC-09c LDR calibration | P4 |
| TC-10 OLED display | P7 |
| TC-11 Mouse movement | P8 |
| TC-12 Mouse buttons | P8 |
| TC-13 Piezo dual-channel | P5 |
| TC-14 Bootmagic | P10 |
| TC-15 Layer switching | P9 |
| TC-16 EEPROM clear | P9, P10 |

### Gate criteria (release)

- [ ] All 16 test cases pass on a fully assembled board (both halves, TRRS connected).
- [ ] Flash ≤ 30 KB; SRAM (.data + .bss) ≤ 1700 bytes.
- [ ] Bootmagic entry works on both halves independently.
- [ ] `QK_BOOT` on ADJUST layer enters bootloader immediately.
- [ ] `EE_CLR` + power-cycle restores all defaults correctly.
- [ ] No USB HID report drops under sustained typing at 120 WPM (measured with a roll-over test tool).
- [ ] No cursor drift when mouse module is absent or powered off.

---

## Appendix A — Flash Budget Tracker

Update after each phase gate.

| Phase | Description | Flash used | Headroom |
| :---- | :---------- | :--------- | :------- |
| P0 | Skeleton | — | — |
| P1 | USB + Matrix | — | — |
| P2 | Split transport | — | — |
| P3 | RGB Matrix | — | — |
| P4 | LDR + calibration | — | — |
| P5 | Piezo | — | — |
| P6 | I2C API | — | — |
| P7 | OLED module | — | — |
| P8 | Mouse module | — | — |
| P9 | Full keymap | — | — |
| P10 | Final | — | ≥ 2 KB required |

---

## Appendix B — Known V-USB Hazards

These constraints apply throughout all phases. Any new driver or callback must be verified against them.

| Rule | Reason |
| :--- | :----- |
| No operation may hold the CPU for > 50 µs | V-USB's INT1 fires every 1.5 µs during USB SOF; a stall causes a USB disconnect |
| Never call I2C from an ISR or `matrix_scan()` | TWI transactions can block up to 2 ms on timeout |
| Timer1 and OC1B (PD4) are reserved for V-USB | PD4 is the D+ line; reconfiguring Timer1 destroys USB |
| Do not use `_delay_ms()` anywhere in the main loop | It busy-waits and will violate the 50 µs rule |
| ADC conversion must be non-blocking (start, then poll) | A blocking wait for `ADIF` can take ~110 µs |
| SPI (WS2812 frame) is safe | Hardware SPI transmits in the background; CPU is not stalled |

---

## Appendix C — Source File Map

Final expected source tree at Phase 10 completion:

```
keyboards/sol/mercurio/rev1/
├── keyboard.json
├── config.h
├── rules.mk
├── mercurio.h              ← split state, custom keycodes
├── mercurio.c              ← keyboard hooks, sync handlers
├── ldr.c / ldr.h           ← ADC, dimming, toggle, calibration
├── piezo.c / piezo.h       ← Timer2 CTC driver
├── piezo_effects.c / .h    ← click, layer, startup effects
├── i2c_module.c / .h       ← generic TWI master API
├── oled_module.c / .h      ← ATtiny48 + SSD1306 controller
├── mouse_module.c / .h     ← ATtiny45 + PSP joystick controller
└── keymaps/
    └── default/
        └── keymap.c        ← BASE, LOWER, RAISE, ADJUST layers
```

---

## Revision History

| Rev | Date       | Changes |
| :-- | :--------- | :------ |
| 1.0 | 2026-03-24 | Initial development plan, phased from FSD rev 1.5 |
