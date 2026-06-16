# Functional Specification Document: sol/mercurio rev1

## ATmega32A Wired Split Keyboard — QMK Firmware

| Field        | Value                   |
| :----------- | :---------------------- |
| Project      | sol                     |
| Board        | mercurio rev1           |
| MCU          | ATmega32A (44-pin TQFP) |
| Firmware     | QMK                     |
| Status       | Draft                   |
| Revision     | 1.5                     |

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [Out of Scope](#2-out-of-scope)
3. [Architecture](#3-architecture)
4. [Applied Technologies](#4-applied-technologies)
5. [Data Models / Schema](#5-data-models--schema)
6. [Development Phases](#6-development-phases)
7. [Functional Requirements](#7-functional-requirements)
8. [Non-Functional Requirements](#8-non-functional-requirements)
9. [Test Cases](#9-test-cases)
10. [Traceability Matrix](#10-traceability-matrix)
11. [Possible Problems (Risk Management)](#11-possible-problems-risk-management)
12. [Appendix](#12-appendix)

---

## 1. System Overview

Mercurio rev1 is a wired, two-piece ergonomic split mechanical keyboard running QMK firmware on an ATmega32A microcontroller. It targets typing comfort and extensibility through full RGB per-key illumination with ambient-adaptive brightness, a PSP-style emulated mouse thumbstick with physical mouse buttons, an OLED status display, and a dual-channel piezo audio driver.

**Primary actors:**

| Actor         | Description                                                                |
| :------------ | :------------------------------------------------------------------------- |
| Typist        | End user operating the keyboard for text input and mouse control           |
| Firmware Dev  | Developer building, configuring, and flashing QMK firmware                 |
| Module Dev    | Developer writing firmware for ATtiny peripheral modules (OLED, mouse)     |

**Master/Slave topology:** Both halves carry a USB connector and the full V-USB circuit. The **Master role is dynamically assigned** at boot: whichever half detects a live USB enumeration (`SPLIT_USB_DETECT`) becomes Master. The Master owns the V-USB stack, aggregates the full keyboard matrix, drives RGB computation, and presents a composite USB HID device (keyboard + mouse) to the host. The opposite half acts as Slave, scanning its local matrix and receiving state synchronization from the Master.

**Left/Right identification:** Because both halves share the same PCB and the same firmware binary, the physical side (left vs. right) is determined via a dedicated handedness pin (`SPLIT_HAND_PIN`). See §3.4.

---

## 2. Out of Scope

The following are explicitly excluded from this revision:

- **Wireless split communication** — inter-half communication is wired TRRS only; Bluetooth or RF is not supported.
- **QMK audio stack** (`AUDIO_ENABLE = no`) — audio feedback is provided by a custom Timer2 CTC piezo driver, not the QMK audio subsystem.

  > **Note:** It is possible to bridge Mercurio's piezo driver into QMK's audio abstraction layer in a future revision. Implementing `audio_driver_initialize_impl()`, `audio_driver_start_impl()`, `audio_driver_stop_impl()`, and calling `audio_update_state()` from a periodic context (e.g. `piezo_task()`) would enable QMK's `PLAY_SONG()`, `NOTE_*` macros, and audio keycodes on top of the existing Timer2 CTC hardware — without modifying the core driver. The QMK AVR PWM driver cannot be used directly: for ATmega32A it only supports D5 via Timer1, which is occupied by V-USB (PD4 = D+).
- **SSD1306 firmware** — the ATtiny48 OLED bridge firmware is a separate AVR-GCC project; its internal rendering logic is outside this specification.
- **ATtiny45 mouse module firmware** — the internal ADC sampling, deadzone, and sensitivity logic on the ATtiny45 is outside this specification; only its I2C register interface is specified here.
- **Bluetooth / console / command interfaces** — `BLUETOOTH_ENABLE`, `COMMAND_ENABLE`, and `CONSOLE_ENABLE` are disabled to conserve flash.
- **LED count finalization** — exact SK6812MINI count per half is TBD pending PCB layout; `LEFT_LED_COUNT` and `RIGHT_LED_COUNT` are PCB-defined constants.
- **Musical sequencing** — the piezo driver plays single notes and predefined short effects only; no multi-note sequencer or MIDI is included.
- **Host-side software** — no companion application, remapper GUI, or Via/Vial support in rev1.

---

## 3. Architecture

### 3.1 System Architecture Overview

Mercurio uses a **split embedded firmware** architecture: two identical PCBs, each running the same firmware binary, operating in a Master/Slave relationship established at runtime. There is no fixed host-side component beyond standard USB HID drivers.

```
┌──────────────────────────────────────┐
│          User Keymaps / Layers       │
├──────────────────────────────────────┤
│        QMK Core (keyboard.h)         │
│  matrix · action · hid · rgbmatrix  │
├────────────────┬─────────────────────┤
│  Split Serial  │   Custom Modules    │
│  (USART driver)│  ldr · i2c_module   │
│                │  piezo              │
├────────────────┴─────────────────────┤
│        AVR HAL (QMK platform layer)  │
│   twi · spi · usart · adc · timer   │
├──────────────────────────────────────┤
│             ATmega32A                │
└──────────────────────────────────────┘
```

### 3.2 Module Decomposition

| Module           | File(s)                         | Runs On | Description                                                     |
| :--------------- | :------------------------------ | :------ | :-------------------------------------------------------------- |
| Matrix scanner   | `matrix.c`                      | Both    | COL2ROW scan of the local 5×7 sub-matrix                        |
| Split transport  | QMK serial (USART driver)       | Both    | Synchronizes matrix + shared state over TRRS                    |
| Shared state     | `mercurio.h` (split_shared_data)| Both    | Layer index, lock LEDs, brightness, volume, audio               |
| RGB Matrix       | QMK rgb_matrix + WS2812 SPI     | Both    | Per-key SK6812MINI drive via hardware SPI                       |
| LDR brightness   | `ldr.c`                         | Both    | Non-blocking ADC poll, scales RGB Matrix value                  |
| I2C Module API   | `i2c_module.c / .h`             | Both    | Generic TWI master driver for plug-in modules                   |
| OLED Controller  | `oled_module.c`                 | Left    | Pushes shared state to ATtiny48 + SSD1306 via TWI               |
| Emulated Mouse   | `mouse_module.c`                | Right   | Polls PSP joystick + 3 buttons via TWI, injects mouse reports   |
| Piezo Driver     | `piezo.c / .h`                  | Both    | Timer2 CTC hardware OC2 toggle; plays key clicks and effects    |

### 3.3 Execution Model

QMK's main loop calls the following user hooks in order, every scan cycle (~1 ms target):

```
housekeeping_task()          ← split state sync arrives here (Master side)
matrix_scan()                ← local key scan
split_transport_task()       ← USART TX/RX
rgb_matrix_task()            ← LED frame computation
ldr_task()         [100 ms]  ← ADC sample, update brightness
oled_module_task() [50 ms]   ← Left: push state to ATtiny48 (I2C)
mouse_module_task()[10 ms]   ← Right: poll PSP module (I2C), queue report
piezo_task()       [1 ms]    ← Both: decrement note duration counter, stop timer when elapsed
```

Timings in brackets indicate the minimum interval between calls (enforced via `timer_elapsed32()`). All I2C and ADC operations are asynchronous or short-blocking; none may hold the CPU for more than ~50 µs to preserve V-USB interrupt timing.

### 3.4 Boot Sequence

```
Power-on
  ├─ Read SPLIT_HAND_PIN (PD2)  →  left or right identity stored
  └─ SPLIT_USB_DETECT probe     →  master or slave role assigned
       ├─ Master: init V-USB, run full QMK loop
       └─ Slave:  wait for USART sync, scan local matrix
```

**Master/Slave detection:**

```c
#define SPLIT_USB_DETECT    // Master = whichever half enumerates on USB
```

`SPLIT_USB_DETECT` probes whether the USB bus is active (D+ pull-up detected) at boot. Because both PCBs carry a USB connector, either half can become Master. This also enables independent firmware flashing of each half via its own USB port (see §6).

**Left/Right handedness detection:**

```c
#define SPLIT_HAND_PIN   D2     // PD2 — determines physical side
// Left half PCB:  PD2 pulled LOW via 10 kΩ to GND  → QMK sees LOW  → left half
// Right half PCB: PD2 floating, internal pull-up    → QMK sees HIGH → right half
```

`SPLIT_HAND_PIN` is read once at boot. QMK's default: LOW = left, HIGH = right. The handedness reading is stored in EEPROM on first valid read.

### 3.5 Hardware Architecture

#### MCU Specification

| Property      | Value                              |
| :------------ | :--------------------------------- |
| Part          | ATmega32A                          |
| Package       | 44-pin TQFP                        |
| Flash         | 32 KB                              |
| SRAM          | 2 KB                               |
| EEPROM        | 1 KB                               |
| Clock Source  | External crystal, 12 MHz           |
| Bootloader    | USBasp-loader (V-USB ISP)          |
| Fuses (CKSEL) | External full-swing crystal, no divide |

> **Clock note:** V-USB requires a crystal-stabilized clock. The internal RC oscillator is not sufficiently accurate. 12 MHz is chosen because it is fully supported by V-USB with published, validated USB timing tables. Do **not** substitute an 8 MHz or 16 MHz crystal without re-validating the V-USB timing constants.

#### Pin Allocation

The table below is authoritative for both halves. Pin numbers refer to the ATmega32A 44-pin TQFP physical pin numbering.

| Component              | Physical Pin       | AVR Port/Pin | Protocol / Function                                                                                  | Half                              |
| :--------------------- | :----------------- | :----------- | :--------------------------------------------------------------------------------------------------- | :-------------------------------- |
| Row 0                  | 37                 | PA0          | GPIO Input (internal pull-up, reads LOW when key pressed)                                            | Both                              |
| Row 1                  | 36                 | PA1          | GPIO Input (internal pull-up)                                                                        | Both                              |
| Row 2                  | 35                 | PA2          | GPIO Input (internal pull-up)                                                                        | Both                              |
| Row 3                  | 34                 | PA3          | GPIO Input (internal pull-up)                                                                        | Both                              |
| Row 4                  | 33                 | PA4          | GPIO Input (internal pull-up)                                                                        | Both                              |
| Column 0               | 21                 | PC2          | GPIO Output (driven LOW to select column)                                                            | Both                              |
| Column 1               | 22                 | PC3          | GPIO Output                                                                                          | Both                              |
| Column 2               | 23                 | PC4          | GPIO Output                                                                                          | Both                              |
| Column 3               | 24                 | PC5          | GPIO Output                                                                                          | Both                              |
| Column 4               | 25                 | PC6          | GPIO Output                                                                                          | Both                              |
| Column 5               | 26                 | PC7          | GPIO Output                                                                                          | Both                              |
| Column 6               | 30                 | PA7          | GPIO Output                                                                                          | Both                              |
| V-USB D−               | 12                 | PD3          | External Interrupt INT1 (V-USB)                                                                      | Both (active on Master half only) |
| V-USB D+               | 13                 | PD4          | GPIO — V-USB data line                                                                               | Both (active on Master half only) |
| USART RX (Split In)    | 9                  | PD0          | Hardware USART0 RX                                                                                   | Both                              |
| USART TX (Split Out)   | 10                 | PD1          | Hardware USART0 TX                                                                                   | Both                              |
| TWI SCL                | 19                 | PC0          | Hardware TWI Clock                                                                                   | Both                              |
| TWI SDA                | 20                 | PC1          | Hardware TWI Data                                                                                    | Both                              |
| SK6812MINI Data        | 1                  | PB5          | Hardware SPI MOSI                                                                                    | Both                              |
| SPI SCK (LED clock)    | 2                  | PB7          | Hardware SPI SCK (driven but not wired out)                                                          | Both                              |
| LDR Sensor             | 31                 | PA6          | ADC Channel 6 (Analog Input)                                                                         | Both                              |
| Split Handedness       | 11                 | PD2          | GPIO Input — `SPLIT_HAND_PIN`; Left: PCB pull-down to GND (LOW); Right: internal pull-up (HIGH)     | Both                              |
| Piezo Driver           | 16                 | PD7          | Timer2 OC2 — CTC hardware toggle for piezo audio                                                    | Both                              |
| Unused / Expansion     | 14–15, 32, 40–43   | PD5,PD6,PA5,PB0–PB3 | Reserved for future use                                                                    | Both                              |

> **Diode direction:** COL2ROW. Columns are driven LOW to select. Row pins are read with internal pull-ups; a pressed key pulls the row LOW through the diode. Diode **anode faces the row pin, cathode faces the column pin**.

#### Physical Connections

**Host interface:** USB connector on **both** halves, each wired to PD3 (D−) and PD4 (D+) with the standard V-USB 3.6 V zener-clamp circuit and 68 Ω series resistors. Only the half with an active USB enumeration runs V-USB; the other half's connector remains idle.

**Handedness pin:** On the **left half PCB**, a 10 kΩ resistor connects PD2 to GND, forcing it LOW. On the **right half PCB**, no external resistor is fitted; the internal pull-up holds PD2 HIGH. Since both halves share the same PCB design, the left-side grounding resistor is populated only on left-half boards.

**Inter-half cable:** 4-pole TRRS.

| TRRS Pole | Signal      |
| :-------- | :---------- |
| Tip       | VCC (5 V)   |
| Ring 1    | GND         |
| Ring 2    | USART TX→RX |
| Sleeve    | USART RX←TX |

---

## 4. Applied Technologies

| Layer               | Technology                        | Justification                                                                                      |
| :------------------ | :-------------------------------- | :------------------------------------------------------------------------------------------------- |
| MCU                 | ATmega32A (44-pin TQFP)           | 32 KB flash, hardware USART/TWI/SPI/ADC/Timer2 — all required peripherals in one chip             |
| USB                 | V-USB (software USB)              | ATmega32A has no hardware USB; V-USB provides validated 12 MHz full-speed USB via INT1             |
| Firmware framework  | QMK                               | Industry-standard keyboard firmware with split keyboard, RGB matrix, pointing device, and USART support |
| Split transport     | QMK USART serial driver           | Hardware USART on ATmega32A provides reliable, interrupt-driven full-duplex split communication   |
| RGB LEDs            | SK6812MINI (WS2812-compatible)    | GRB addressable LEDs with hardware SPI bit-stream generation at 12 MHz                            |
| LED driver          | WS2812 SPI driver (QMK)           | Uses hardware SPI peripheral; CPU not stalled during LED frame transmission                        |
| Ambient sensor      | LDR voltage divider + ADC6        | Passive, low-cost ambient light sensing via the ATmega32A's built-in 10-bit ADC                   |
| I2C modules         | Hardware TWI @ 100 kHz            | Dedicated local TWI bus per half; electrically isolated between halves to prevent bus conflicts    |
| OLED bridge         | ATtiny48 + SSD1306                | Offloads SSD1306 framebuffer rendering to a dedicated co-processor; isolates display timing from main loop |
| Mouse module        | ATtiny45 + PSP joystick           | Dedicated co-processor for ADC sampling and deadzone filtering; presents clean int8 deltas via I2C |
| Audio               | Timer2 CTC, OC2 hardware toggle   | Zero ISR overhead during tone playback; Timer2/OC2 (PD7) is the only AVR timer output safe from V-USB pin conflicts |
| Bootloader          | USBasp-loader (V-USB ISP)         | Enables USB-only firmware flashing post-install; compatible with `avrdude` and standard USBasp tooling |
| Build toolchain     | QMK CLI (`qmk compile / flash`)   | Standard QMK build environment; AVR-GCC cross-compiler                                            |

---

## 5. Data Models / Schema

### 5.1 Split Shared State

Synchronized from Master to Slave on every USART transport cycle.

```c
// mercurio_split_state.h
typedef struct {
    uint8_t  active_layer;      // 0–15
    uint8_t  led_brightness;    // 0–100 (percentage, post-LDR scaling)
    uint8_t  audio_volume;      // 0–100
    bool     caps_lock;
    bool     num_lock;
    bool     scroll_lock;
    uint8_t  piezo_ocr2;        // Timer2 OCR2 value for slave piezo note (0 = silent)
    uint8_t  piezo_duration_ms; // Duration in ms for slave piezo note
    bool     ldr_enabled;       // LDR auto-dimming active (toggled by LDR_TOG, persisted to EEPROM)
    int8_t   ldr_cal_offset;    // Slave's LDR calibration offset in ADC units (signed); Master writes, Slave saves to EEPROM on change
} mercurio_split_state_t;

extern mercurio_split_state_t mercurio_state;
#define SPLIT_TRANSACTION_IDS_USER MERCURIO_STATE_SYNC
```

### 5.2 I2C Module Common Register Map

All modules implement the following base registers:

| Register | ID     | Direction | Length | Description                             |
| :------- | :----- | :-------- | :----- | :-------------------------------------- |
| IDENT    | `0x00` | R         | 2      | `[MODULE_TYPE, FW_VERSION]`             |
| STATUS   | `0x01` | R         | 1      | Bit 0: ready; Bit 1: error; Bit 2: busy |
| RESET    | `0xFF` | W         | 0      | Soft-reset module                       |

**MODULE_TYPE values:**

| Value  | Module          |
| :----- | :-------------- |
| `0x01` | OLED Controller |
| `0x02` | Emulated Mouse  |

### 5.3 OLED Module Register Map (Left half, addr `0x20`)

| Register       | ID     | Direction | Payload                                      |
| :------------- | :----- | :-------- | :------------------------------------------- |
| SET_LAYER      | `0x10` | W         | `uint8_t layer` (0–15)                       |
| SET_CAPS       | `0x11` | W         | `uint8_t state` (0 = off, 1 = on)            |
| SET_BRIGHTNESS | `0x12` | W         | `uint8_t pct` (0–100)                        |
| SET_VOLUME     | `0x13` | W         | `uint8_t pct` (0–100)                        |
| SET_STATE_BULK | `0x14` | W         | 5 bytes: `[layer, caps, brightness, volume, flags]` — bit 0 of `flags` = `ldr_enabled` |
| REFRESH        | `0x15` | W         | 0 bytes — force immediate display update     |
| GET_DISPLAY    | `0x16` | R         | 512 bytes — raw framebuffer (diagnostic only)|
| SET_LDR_CAL    | `0x17` | W         | `int8_t offset` — LDR calibration offset (ADC units) for the local (left) half; displayed on OLED as e.g. `CAL:+12` |

> **Preferred update path:** Use `SET_STATE_BULK` (single 5-byte write) for the common state fields; send `SET_LDR_CAL` only when the calibration offset changes to avoid unnecessary TWI traffic.

### 5.4 Mouse Module Register Map (Right half, addr `0x21`)

| Register        | ID     | Direction | Payload                                                             |
| :-------------- | :----- | :-------- | :------------------------------------------------------------------ |
| GET_AXES        | `0x10` | R         | 4 bytes: `[int8_t dx, int8_t dy, uint8_t buttons, uint8_t status]` |
| SET_DEADZONE    | `0x11` | W         | `uint8_t radius` (raw ADC units, 0–127; default: 20)               |
| SET_SENSITIVITY | `0x12` | W         | `uint8_t level` (1–10; default: 5)                                 |
| GET_RAW         | `0x13` | R         | 4 bytes: `[uint16_t raw_x, uint16_t raw_y]` (diagnostic)           |

**`buttons` bitmask:**

| Bit | Button       |
| :-- | :----------- |
| 0   | Stick click  |
| 1   | Left click   |
| 2   | Right click  |
| 3–7 | Reserved (0) |

### 5.5 Piezo Note Table (Timer2 prescaler /64, f_timer = 187.5 kHz)

| Constant              | Target Freq | OCR2 | Actual Freq |
| :-------------------- | :---------- | :--- | :---------- |
| `PIEZO_NOTE_440HZ`    | 440 Hz      | 212  | 440 Hz      |
| `PIEZO_NOTE_1KHZ`     | 1000 Hz     | 92   | 1010 Hz     |
| `PIEZO_NOTE_2KHZ`     | 2000 Hz     | 46   | 1976 Hz     |
| `PIEZO_NOTE_4KHZ`     | 4000 Hz     | 22   | 3978 Hz     |

**Frequency formula:**

```
f_out = 187500 / (2 × (OCR2 + 1))   [Hz]
OCR2  = (187500 / (2 × f_out)) − 1
```

---

## 6. Development Phases

### Phase 1 — Hardware Bring-Up (ISP-based)

Direct ISP flashing via USBasp programmer. No bootloader required. Both halves are flashed with the identical firmware binary; Master/Slave and Left/Right roles are determined at runtime.

**Prerequisites:**
- QMK toolchain (`qmk setup` completed)
- USBasp programmer hardware
- `avrdude` ≥ 6.3

**First-time fuse programming** (ATmega32A ships with internal RC oscillator):

```bash
avrdude -p m32 -c usbasp \
  -U lfuse:w:0xFF:m \
  -U hfuse:w:0xD9:m
```

> **Warning:** Incorrect fuse settings can lock the MCU. Verify against the ATmega32A datasheet Table 2 before writing.

**Build and flash:**

```bash
# Build
qmk compile -kb sol/mercurio/rev1 -km default
# Output: .build/sol_mercurio_rev1_default.hex

# Flash either half (binary is identical)
avrdude -p m32 -c usbasp \
  -U flash:w:.build/sol_mercurio_rev1_default.hex:i

# Verify
avrdude -p m32 -c usbasp \
  -U flash:v:.build/sol_mercurio_rev1_default.hex:i
```

**Flash ATtiny modules** (separate AVR-GCC projects):

```bash
avrdude -p t48 -c usbasp -U flash:w:oled_module.hex:i   # ATtiny48 (OLED)
avrdude -p t45 -c usbasp -U flash:w:mouse_module.hex:i  # ATtiny45 (Mouse)
```

### Phase 2 — Bootloader Install (one-time, ISP required)

Flash the USBasp-loader bootloader. After this, Phase 3 applies for all subsequent firmware updates.

```bash
avrdude -p m32 -c usbasp \
  -U flash:w:usbasploader_m32a.hex:i
```

### Phase 3 — Production Firmware Updates (USB-only)

Once the bootloader is installed, use Bootmagic or `QK_BOOT` — no ISP programmer required.

#### Updating via Bootmagic

1. Unplug both halves from USB and TRRS.
2. Hold the **top-left key** on the half to be updated.
3. Plug USB into that half's connector.
4. Release the key — the half enters USBasp bootloader.
5. Flash with `avrdude` as in Phase 1.

> **Key insight:** `SPLIT_USB_DETECT` (no `MASTER_RIGHT`) enables this for both halves independently. The Master role follows the USB connection, making each half a self-sufficient USB device capable of running the bootloader.

#### Updating via QK_BOOT keycode

```
Hold: MO(_ADJUST) + QK_BOOT  →  keyboard enters USBasp bootloader immediately
```

---

## 7. Functional Requirements

### Actors

| Actor        | Description                                     |
| :----------- | :---------------------------------------------- |
| Typist       | End user performing keyboard and mouse input    |
| Firmware Dev | Developer building and flashing QMK firmware    |
| Module Dev   | Developer maintaining ATtiny peripheral modules |

### Requirements

**FR-01:** The system shall scan a 5-row × 7-column key matrix per half using COL2ROW diode orientation, driving each column LOW and reading all 5 row pins with internal pull-ups.

**FR-02:** The system shall apply QMK `sym_defer_g` debounce with a 5 ms threshold to all key events on both halves.

**FR-03:** The system shall communicate between halves via a full-duplex hardware USART link over the TRRS cable at 115200 bps.

**FR-04:** The system shall dynamically assign the Master role at boot to whichever half detects an active USB enumeration (`SPLIT_USB_DETECT`), with no hard-coded side preference.

**FR-05:** The system shall determine left/right physical identity via `SPLIT_HAND_PIN` (PD2): LOW = left (external 10 kΩ pull-down on left PCB), HIGH = right (internal pull-up).

**FR-06:** The system shall synchronize a `mercurio_split_state_t` struct (layer, brightness, lock LEDs, piezo note parameters) from Master to Slave on every USART transport cycle.

**FR-07:** The system shall present a USB composite HID device with two interfaces: Interface 0 (Keyboard, 6KRO) and Interface 1 (Mouse, X/Y/5-button), with `MOUSE_SHARED_EP = no` to ensure the mouse has its own endpoint.

**FR-08:** The system shall drive per-key SK6812MINI RGB LEDs on both halves via hardware SPI (MOSI on PB5), using the QMK `WS2812 SPI` driver with GRB byte order.

**FR-09:** The system shall sample the LDR voltage divider on ADC channel 6 (PA6) every 100 ms and, when LDR auto-dimming is enabled, scale the RGB Matrix brightness (`rgb_matrix_set_val()`) proportionally, with a minimum floor of 20% brightness and 5-step hysteresis to prevent flicker. Before normalization, each half shall apply its locally-stored signed calibration offset (`ldr_cal_offset`) to the raw ADC reading, clamped to [0, 1023], to compensate for LDR sensitivity differences between the two halves (see FR-18). When disabled, `ldr_task()` shall skip the `rgb_matrix_set_val()` call, leaving brightness under manual control. The enabled/disabled state shall be persisted to EEPROM (defaulting to enabled on first boot) and restored at power-on.

**FR-10:** The system shall provide a generic I2C Module API (`i2c_module_init`, `i2c_module_write`, `i2c_module_read`) with a 2 ms transaction timeout, usable by any TWI slave module on either half's local bus.

**FR-11:** The system shall push keyboard state (layer, caps lock, brightness, volume, ldr_enabled) to the ATtiny48 OLED module on the left half via TWI (addr `0x20`) every 50 ms using the `SET_STATE_BULK` register, triggering a display refresh on the SSD1306. The OLED shall display an LDR auto-dimming indicator (e.g. `LDR ON` / `LDR OFF`) that reflects the current state of the toggle.

**FR-12:** The system shall poll the ATtiny45 mouse module on the right half via TWI (addr `0x21`) every 10 ms using `GET_AXES`, and inject a `report_mouse_t` pointing device report when movement or button state changes are detected.

**FR-13:** The system shall drive a passive piezo transducer on each half via Timer2 CTC mode with hardware OC2 toggle (PD7), with tone frequency set by OCR2 and duration managed by a per-ms counter decrement in `piezo_task()`.

**FR-14:** The system shall transmit the Slave's piezo note parameters (`piezo_ocr2`, `piezo_duration_ms`) via the split shared state, enabling both halves to play complementary notes simultaneously within ~1 ms of each other.

**FR-15:** The system shall support Bootmagic Lite: holding the top-left key (matrix position row 0, col 0) during power-on shall invoke `bootloader_jump()`, entering the USBasp bootloader.

**FR-16:** The system shall implement a 4-layer keymap (`_BASE`, `_LOWER`, `_RAISE`, `_ADJUST`) accessible via dedicated layer keys on Row 4 of each half.

**FR-17:** The `_ADJUST` layer shall expose firmware control keys: `RGB_TOG`, `RGB_MOD`, `RGB_HUI`, `RGB_SAI`, `LDR_TOG`, `LDR_CAL_UP`, `LDR_CAL_DN`, `EE_CLR`, and `QK_BOOT`. `LDR_TOG` shall toggle the LDR auto-dimming enabled flag (FR-09), immediately save the new state to EEPROM, update `mercurio_state.ldr_enabled`, and trigger an OLED refresh on the next `oled_module_task()` cycle. `LDR_CAL_UP` and `LDR_CAL_DN` shall invoke the LDR calibration procedure described in FR-18.

**FR-18:** The system shall provide per-half LDR calibration via two custom keycodes (`LDR_CAL_UP`, `LDR_CAL_DN`) on the `_ADJUST` layer. Pressing a calibration key adjusts the `ldr_cal_offset` of the physical half on which the key is pressed (determined by `record->event.key.row < MATRIX_ROWS / 2`), by `±LDR_CAL_STEP` ADC units per press, clamped to `[−LDR_CAL_MAX, +LDR_CAL_MAX]`. The adjusted offset is immediately saved to the local half's EEPROM. For the Slave half, the Master additionally updates `mercurio_state.ldr_cal_offset` so the new value propagates via USART on the next sync cycle; the Slave detects the change in `housekeeping_task_slave()` and saves it to its own EEPROM. The current calibration offset shall be sent to the OLED module via the `SET_LDR_CAL` register whenever it changes, displayed as a signed decimal value (e.g. `CAL:+12`).

---

## 8. Non-Functional Requirements

**NFR-01 — V-USB ISR Timing:**
All peripheral drivers must be non-blocking. No driver or callback may hold the CPU for more than ~50 µs. I2C transactions are bounded by `I2C_MODULE_TIMEOUT_MS = 2 ms` and called only from `housekeeping_task()`, never from an ISR. V-USB INT1 (PD3) fires every 1.5 µs during USB SOF packets.

**NFR-02 — Flash Budget:**
Total compiled firmware must not exceed 32 KB. `BLUETOOTH_ENABLE`, `COMMAND_ENABLE`, and `CONSOLE_ENABLE` are disabled. `AUDIO_ENABLE` is disabled in favour of the custom piezo driver.

**NFR-03 — USB Power:**
`USB_MAX_POWER_CONSUMPTION` is set to 100 mA. LDR-based brightness dimming provides indirect current reduction for the SK6812MINI chain.

**NFR-04 — Scan Cycle Latency:**
Main loop scan cycle target is ~1 ms. All module polling tasks are rate-limited via `timer_elapsed32()` to avoid starving the matrix scan or V-USB service.

**NFR-05 — I2C Module Response:**
Modules must respond within 1 ms of an I2C START condition. The Mercurio-side timeout is 2 ms (`I2C_MODULE_TIMEOUT_MS`). On timeout, the mouse report is zeroed (no phantom motion).

**NFR-06 — Clock Accuracy:**
The system shall use a 12 MHz external crystal. The internal RC oscillator is not permitted; V-USB timing is only validated at crystal-stabilized 12 MHz.

**NFR-07 — Piezo Timer Safety:**
Timer2/OC2 (PD7) is the only AVR timer output available for piezo audio. Timer1/OC1B (PD4) must not be used as PD4 is the V-USB D+ line.

**NFR-08 — Piezo CPU Budget:**

| Event                 | CPU Cycles | Duration @ 12 MHz | Compatible with V-USB? |
| :-------------------- | :--------- | :---------------- | :--------------------- |
| OC2 hardware toggle   | 0          | 0 µs (hardware)   | Yes                    |
| `piezo_task()` / cycle| ~8 cycles  | ~0.7 µs           | Yes                    |
| Start/stop note       | ~15 cycles | ~1.25 µs          | Yes                    |

---

## 9. Test Cases

**TC-01 — Matrix Scan Completeness**
Steps: Press each key individually across all 5 rows × 7 columns on each half. Observe HID reports on host.
Expected: Each key position generates a unique, correct keycode with no ghosting or missed events.

**TC-02 — Debounce Validation**
Steps: Simulate contact bounce on a row pin within a 4 ms window (oscilloscope or bench switch). Observe HID report count.
Expected: Exactly one key-down event is registered per intentional press; bounces within 5 ms window are suppressed.

**TC-03 — Split USART Communication**
Steps: Connect both halves via TRRS. Press keys on the Slave half while USB is connected to the Master.
Expected: Slave key presses are reported to the host within one scan cycle (~1 ms) with correct key positions (rows offset by 5).

**TC-04 — Dynamic Master/Slave Assignment**
Steps: Connect USB to the left half only. Verify Master role. Then reconnect USB to the right half only. Verify Master role.
Expected: Whichever half USB is connected to becomes Master and presents the full HID device; the other half acts as Slave.

**TC-05 — Left/Right Handedness**
Steps: Power on both halves. Verify left half reports keys on rows 0–4 and right half on rows 5–9 in QMK's unified matrix.
Expected: Layer maps, OLED module (left), and mouse module (right) are assigned to the correct physical side.

**TC-06 — Split Shared State Propagation**
Steps: Activate `_LOWER` layer on the Master half. Observe OLED display on the left half within 50 ms.
Expected: OLED displays `Layer: 1` (or equivalent) within one `oled_module_task()` cycle.

**TC-07 — USB Composite HID Device**
Steps: Connect Master half to a host. Open Device Manager / `lsusb`. Attempt text input and mouse movement.
Expected: Host enumerates two HID interfaces (keyboard + mouse); both function independently.

**TC-08 — RGB Matrix Illumination**
Steps: Cycle through RGB animations using `RGB_MOD` on the `_ADJUST` layer. Toggle with `RGB_TOG`.
Expected: All per-key LEDs respond correctly; `RGB_TOG` disables and re-enables the full matrix.

**TC-09 — LDR Ambient Dimming**
Steps: Cover the LDR sensor fully (darkness). Observe LED brightness over ~200 ms. Uncover sensor.
Expected: LED brightness decreases to the floor value (≥20%) in darkness and increases in ambient light; no rapid flickering.

**TC-09c — LDR Per-Half Calibration**
Steps: (1) With both halves connected, press `LDR_CAL_UP` 3× on the left half while in `_ADJUST`. Power-cycle. (2) Press `LDR_CAL_UP` 3× on the right half while in `_ADJUST`. Power-cycle. (3) Press `EE_CLR` and power-cycle.
Expected: (1) Left half brightness curve shifts by +3 × `LDR_CAL_STEP` ADC units; OLED displays updated `CAL:` value; offset persists after power-cycle. (2) Right half brightness curve shifts independently; left half is unaffected. (3) Both offsets reset to 0 after `EE_CLR` and power-cycle.

**TC-09b — LDR Toggle**
Steps: (1) Press `LDR_TOG` on the `_ADJUST` layer while LDR auto-dimming is enabled. Cover and uncover the LDR sensor. (2) Power-cycle the keyboard. (3) Press `LDR_TOG` again to re-enable.
Expected: (1) LED brightness no longer tracks the LDR; brightness remains at its last manual value. (2) After power-cycle, auto-dimming remains disabled (EEPROM-persisted). (3) Brightness resumes tracking the LDR within the next 100 ms `ldr_task()` cycle.

**TC-10 — OLED Status Display**
Steps: Change layers, toggle Caps Lock, press `LDR_TOG`, press `LDR_CAL_UP` on the left half, observe the 128×32 SSD1306 display.
Expected: Layer index and Caps Lock indicator update within 50 ms; brightness and volume values reflect current state; LDR indicator toggles between `LDR ON` and `LDR OFF` within one `oled_module_task()` cycle (~50 ms); calibration offset updates to `CAL:+N` within one `oled_module_task()` cycle.

**TC-11 — Emulated Mouse Movement**
Steps: Tilt the PSP analog thumbstick in all four cardinal directions and diagonals on the right half.
Expected: Host cursor moves in the corresponding direction; movement stops when stick returns to center (deadzone applied).

**TC-12 — Mouse Button Input**
Steps: Press left click, right click, and stick-click buttons on the mouse module.
Expected: Host registers corresponding mouse button events; all three buttons function independently.

**TC-13 — Piezo Dual-Channel Keyclick**
Steps: Press any key on either half while piezo transducers are populated.
Expected: Both halves emit an audible click simultaneously; left half at 1 kHz, right half at 2 kHz; duration ~3 ms.

**TC-14 — Bootmagic Bootloader Entry**
Steps: Unplug USB and TRRS. Hold top-left key (row 0, col 0). Plug USB into that half.
Expected: Half enumerates as a USBasp device on the host within ~2 seconds; `avrdude` can successfully flash firmware.

**TC-15 — Layer Switching**
Steps: Hold `LOWER` key, verify symbol layer. Hold `RAISE` key, verify navigation layer. Hold both for `_ADJUST`.
Expected: All layer keys activate their respective layers; releasing returns to `_BASE`.

**TC-16 — EEPROM Clear**
Steps: Configure a custom RGB setting. Press `EE_CLR` on the `_ADJUST` layer. Power-cycle.
Expected: RGB settings, stored layer state, and LDR calibration revert to firmware defaults after power-cycle.

---

## 10. Traceability Matrix

| Requirement ID | Requirement Description                            | Priority | Test Case ID |
| :------------- | :------------------------------------------------- | :------- | :----------- |
| FR-01          | COL2ROW 5×7 matrix scan per half                  | Must     | TC-01        |
| FR-02          | sym_defer_g debounce, 5 ms threshold               | Must     | TC-02        |
| FR-03          | Full-duplex USART split at 115200 bps              | Must     | TC-03        |
| FR-04          | Dynamic Master/Slave via SPLIT_USB_DETECT          | Must     | TC-04        |
| FR-05          | Left/Right handedness via SPLIT_HAND_PIN (PD2)    | Must     | TC-05        |
| FR-06          | Split shared state sync (mercurio_split_state_t)  | Must     | TC-06        |
| FR-07          | USB composite HID (keyboard + mouse, separate EP)  | Must     | TC-07        |
| FR-08          | SK6812MINI RGB matrix via hardware SPI             | Must     | TC-08        |
| FR-09          | LDR ambient brightness auto-dimming with toggle and EEPROM persistence | Should   | TC-09, TC-09b |
| FR-10          | Generic I2C Module API with 2 ms timeout           | Must     | TC-10, TC-11 |
| FR-11          | OLED state display via ATtiny48 (left half) including LDR indicator | Should   | TC-10        |
| FR-12          | Emulated mouse via ATtiny45 + PSP joystick (right) | Should   | TC-11, TC-12 |
| FR-13          | Piezo Timer2 CTC audio driver                      | Should   | TC-13        |
| FR-14          | Dual-channel piezo sync via split shared state     | Should   | TC-13        |
| FR-15          | Bootmagic Lite bootloader entry                    | Must     | TC-14        |
| FR-16          | 4-layer keymap (BASE / LOWER / RAISE / ADJUST)     | Must     | TC-15        |
| FR-17          | ADJUST layer firmware controls incl. LDR_TOG/CAL   | Must     | TC-09b, TC-09c, TC-10, TC-15, TC-16 |
| FR-18          | Per-half LDR calibration offset (EEPROM, OLED)    | Should   | TC-09c, TC-10 |

---

## 11. Possible Problems (Risk Management)

| Risk                                         | Likelihood | Impact | Mitigation                                                                                                       |
| :------------------------------------------- | :--------- | :----- | :--------------------------------------------------------------------------------------------------------------- |
| V-USB timing violation from blocking driver  | Medium     | High   | Enforce ≤50 µs CPU hold rule in all drivers; I2C timeout at 2 ms with guard in `housekeeping_task()` only        |
| Flash overflow (>32 KB)                      | Medium     | High   | Disable all unused QMK features in `rules.mk`; monitor `.build/*.map` size; AUDIO_ENABLE is already disabled     |
| RC oscillator clock drift breaks V-USB       | Low        | High   | Crystal is mandatory; document fuse settings; add fuse verification step in Phase 1 checklist                    |
| I2C module timeout causes phantom mouse input | Medium    | Medium | `mouse_module_task()` zeros the mouse report on timeout (FR-12); STATUS register bit 0 verified before injecting |
| TRRS hot-plug causes USART framing errors    | High       | Low    | QMK serial driver auto-resynchronizes; recommend plugging TRRS before USB at each power-on                       |
| Left/right handedness stored incorrectly in EEPROM | Low | Medium | `EE_CLR` on ADJUST layer resets stored handedness; recommend clearing EEPROM on first flash                     |
| Timer1/OC1B (PD4) accidentally used for audio | Low      | High   | PD4 is V-USB D+; explicitly documented in §NFR-07; use Timer2/OC2 (PD7) only                                    |
| ATtiny45 boot calibration drift              | Low        | Medium | Center calibration re-runs at each power-on (16-sample average); deadzone absorbs minor drift                    |
| ~1 ms USART latency on stereo piezo effects  | High       | Low    | Latency is inaudible for percussive effects (click, tap); acceptable for the intended use-case                   |
| SK6812MINI LED count undefined at compile time | High     | Low    | `LEFT_LED_COUNT` / `RIGHT_LED_COUNT` are PCB-defined constants; firmware compiles only after PCB layout finalized |

---

## 12. Appendix

### A. QMK Configuration Reference

#### `rules.mk`

```makefile
# MCU
MCU = atmega32a
F_CPU = 12000000
BOOTLOADER = usbasploader

# Core features
SPLIT_KEYBOARD = yes
SERIAL_DRIVER = usart

# USB HID
MOUSEKEY_ENABLE = yes
EXTRAKEY_ENABLE = yes
POINTING_DEVICE_ENABLE = yes
POINTING_DEVICE_DRIVER = custom
MOUSE_SHARED_EP = no

# RGB
RGB_MATRIX_ENABLE = yes
WS2812_DRIVER = spi

# Bootmagic (hold top-left key at power-on to enter bootloader)
BOOTMAGIC_ENABLE = yes

# Audio: custom piezo driver on Timer2/OC2, not QMK audio stack
AUDIO_ENABLE = no

# Custom modules
SRC += ldr.c i2c_module.c oled_module.c mouse_module.c piezo.c

# Disable unused features to fit in 32 KB flash
BLUETOOTH_ENABLE = no
COMMAND_ENABLE = no
CONSOLE_ENABLE = no
```

#### `config.h`

```c
#pragma once

// Identity
#define VENDOR_ID       0x534C      // "SL" (sol)
#define PRODUCT_ID      0x0001      // mercurio rev1
#define DEVICE_VER      0x0001
#define MANUFACTURER    "sol"
#define PRODUCT         "mercurio rev1"

// USB power
#define USB_MAX_POWER_CONSUMPTION 100

// Matrix
#define MATRIX_ROWS         10      // 5 rows × 2 halves
#define MATRIX_COLS          7
#define MATRIX_ROW_PINS      { A0, A1, A2, A3, A4 }
#define MATRIX_COL_PINS      { C2, C3, C4, C5, C6, C7, A7 }
#define DIODE_DIRECTION      COL2ROW

// Split — dynamic master via USB detection; handedness via dedicated pin
#define SPLIT_USB_DETECT
#define SPLIT_HAND_PIN          D2  // PD2: left=LOW (PCB pull-down), right=HIGH (pull-up)
#define SERIAL_USART_SPEED      115200
#define SERIAL_USART_TX_PIN     D1
#define SERIAL_USART_RX_PIN     D0
#define SERIAL_USART_FULL_DUPLEX

// Split shared state
#define SPLIT_TRANSACTION_IDS_USER MERCURIO_STATE_SYNC

// RGB
#define WS2812_DI_PIN                 B5
#define WS2812_BYTE_ORDER             WS2812_BYTE_ORDER_GRB
#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 200
#define RGB_MATRIX_SPLIT              { LEFT_LED_COUNT, RIGHT_LED_COUNT }

// LDR — voltage divider: VCC → LDR(20kΩ) → PA6 → 10kΩ → GND
#define LDR_ENABLE
#define LDR_POLL_INTERVAL_MS    100
#define LDR_ADC_DARK             80   // Near-total darkness (LDR >> 10k, V_ADC near 0)
#define LDR_ADC_BRIGHT          900   // Bright office/direct light (LDR ~ 1k)
#define LDR_MIN_BRIGHTNESS       20
#define LDR_DEFAULT_ENABLED      true   // LDR auto-dimming on by default; toggled by LDR_TOG
#define LDR_CAL_STEP             4      // ADC units per LDR_CAL_UP / LDR_CAL_DN press
#define LDR_CAL_MAX              127    // Maximum calibration offset magnitude (±127 ADC units)

// I2C
#define I2C_DRIVER_CONFIG       { .speed = I2C_SPEED_STANDARD }   // 100 kHz
#define I2C_MODULE_TIMEOUT_MS   2
#define OLED_MODULE_ADDR        0x20
#define MOUSE_MODULE_ADDR       0x21

// Piezo — Timer2 CTC, OC2 = PD7 (pin 16)
#define PIEZO_PIN                D7
#define PIEZO_PRESCALER          64      // Timer2 CS22:CS20 = 0b100 → clk/64 = 187.5 kHz
#define PIEZO_NOTE_440HZ         212     // A4 ~440 Hz
#define PIEZO_NOTE_1KHZ           92     // ~1 kHz
#define PIEZO_NOTE_2KHZ           46     // ~2 kHz
#define PIEZO_NOTE_4KHZ           22     // ~4 kHz
#define PIEZO_CLICK_DURATION_MS    3     // Key-click pulse duration
#define PIEZO_LAYER_TONE_MS       30     // Per-note duration for layer-change effect

// Debounce
#define DEBOUNCE 5

// Bootmagic key position (top-left key of each half = row 0, col 0)
#define BOOTMAGIC_LITE_ROW      0
#define BOOTMAGIC_LITE_COLUMN   0
```

#### `keyboard.json` (excerpt)

```json
{
    "keyboard_name": "mercurio",
    "manufacturer": "sol",
    "processor": "atmega32a",
    "bootloader": "usbasploader",
    "usb": {
        "vid": "0x534C",
        "pid": "0x0001",
        "device_version": "0.0.1",
        "max_power": 100
    },
    "features": {
        "bootmagic": true,
        "mousekey": true,
        "extrakey": true,
        "split_keyboard": true,
        "rgb_matrix": true,
        "pointing_device": true
    },
    "split": {
        "enabled": true,
        "serial": { "driver": "usart" },
        "handedness": { "pin": "D2" }
    },
    "ws2812": {
        "pin": "B5",
        "driver": "spi"
    },
    "matrix_pins": {
        "rows": ["A0", "A1", "A2", "A3", "A4"],
        "cols": ["C2", "C3", "C4", "C5", "C6", "C7", "A7"]
    },
    "diode_direction": "COL2ROW"
}
```

### B. C API Reference

#### I2C Module API (`i2c_module.h`)

```c
#define I2C_MODULE_TIMEOUT_MS  2

typedef enum {
    MODULE_OK      = 0,
    MODULE_ERROR   = 1,
    MODULE_BUSY    = 2,
    MODULE_TIMEOUT = 3,
} module_status_t;

// Initialise a module slot and verify it responds with the expected type.
module_status_t i2c_module_init(uint8_t addr, uint8_t expected_type);

// Write a register (with optional payload).
module_status_t i2c_module_write(uint8_t addr, uint8_t reg, const uint8_t *data, uint8_t len);

// Read a register response.
module_status_t i2c_module_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);
```

#### Piezo API (`piezo.h`)

```c
// Initialise Timer2 and PD7; leaves output silent.
void piezo_init(void);

// Play a single note. ocr2: frequency selector (0 = silence); duration_ms: 1–255 ms.
void piezo_play(uint8_t ocr2, uint8_t duration_ms);

// Stop immediately (called internally when duration expires).
void piezo_stop(void);

// Call once per ms scan cycle.
void piezo_task(void);
```

#### Piezo Effects (`piezo_effects.h`)

```c
// Single 2 kHz burst — called from process_record_kb() on key-down
void piezo_effect_keyclick(void);

// Ascending two-tone — called from layer_state_set_kb()
void piezo_effect_layer_change(uint8_t new_layer);

// Rising sweep at power-on — called from keyboard_post_init_kb()
void piezo_effect_startup(void);
```

**Effect parameters:**

| Effect            | Left Half (Slave)         | Right Half (Master)        | Duration   |
| :---------------- | :------------------------ | :------------------------- | :--------- |
| Key click         | 1 kHz (`PIEZO_NOTE_1KHZ`) | 2 kHz (`PIEZO_NOTE_2KHZ`)  | 3 ms       |
| Layer change (up) | 880 Hz → 1760 Hz          | 1 kHz → 2 kHz              | 30 ms/note |
| Startup           | 440 Hz → 880 Hz → 1760 Hz | 500 Hz → 1 kHz → 2 kHz    | 80 ms/note |

**Dual-channel synchronization:**

```c
// Master side
void piezo_effect_keyclick(void) {
    piezo_play(PIEZO_NOTE_2KHZ, PIEZO_CLICK_DURATION_MS);
    mercurio_state.piezo_ocr2        = PIEZO_NOTE_1KHZ;
    mercurio_state.piezo_duration_ms = PIEZO_CLICK_DURATION_MS;
}

// Slave side (in housekeeping_task, after sync received)
void mercurio_sync_slave_handler(void) {
    if (mercurio_state.piezo_ocr2 > 0) {
        piezo_play(mercurio_state.piezo_ocr2, mercurio_state.piezo_duration_ms);
    }
}
```

The ~1 ms USART latency between Master and Slave note onset is inaudible for percussive effects.

### C. Piezo Hardware Circuit

**Option A — Direct GPIO drive (simpler, lower volume):**

```
PD7 (OC2) ──100 Ω──┬── Piezo (+)
                    │
                   Piezo (−) ──── GND
```

**Option B — NPN transistor drive (recommended, louder):**

```
VCC (5V) ────────────────── Piezo (+)
                                │
                            Piezo (−)
                                │
                           Collector (NPN: BC547 / 2N3904)
PD7 (OC2) ──1kΩ──── Base
                           Emitter ──── GND
```

- Base resistor: 1 kΩ (limits base current to ~4 mA; fully saturates NPN)
- Piezo: passive buzzer, 5–27 mm diameter, 3–5 V rated
- No flyback diode needed (passive piezo is purely capacitive)

### D. LDR Voltage Divider

```
VCC (5V)
   │
  LDR (~20 kΩ nominal, decreases in brighter light)
   │
  PA6 ──── ADC Channel 6
   │
  10 kΩ
   │
  GND
```

**Approximate ADC range:**

| Condition     | LDR approx. | V at PA6 | ADC10 (0–1023) |
| :------------ | :---------- | :------- | :------------- |
| Total darkness| > 100 kΩ    | < 0.5 V  | < 100          |
| Dim room      | ~20 kΩ      | 1.67 V   | ~342           |
| Office light  | ~5 kΩ       | 3.33 V   | ~682           |
| Bright/direct | ~1 kΩ       | 4.55 V   | ~930           |

**Algorithm:**

```
ldr_task() runs every 100 ms:
  1. Start ADC conversion on channel 6 (non-blocking).
  2. On next call, if ADCSRA.ADIF is set, read ADCW.
  3. If mercurio_state.ldr_enabled is false: return (skip brightness update).
  4. Apply local calibration: adc_cal = clamp(ADC_raw + ldr_cal_local, 0, 1023)
       where ldr_cal_local is the EEPROM-stored offset for this half.
  5. Normalize: brightness_pct = map(adc_cal, LDR_ADC_DARK, LDR_ADC_BRIGHT, LDR_MIN_BRIGHTNESS, 100)
  6. Apply 5-step hysteresis to prevent flicker.
  7. Call rgb_matrix_set_val(brightness_pct * RGB_MATRIX_MAXIMUM_BRIGHTNESS / 100).
  8. Update mercurio_state.led_brightness for OLED display.
```

**LDR toggle (called from `process_record_kb()` on `LDR_TOG` key-down):**

```
ldr_toggle():
  1. Flip mercurio_state.ldr_enabled.
  2. Write new state to EEPROM (eeconfig slot or custom EEPROM address).
  3. State propagates to Slave on next USART sync cycle.
  4. OLED reflects new state within the next oled_module_task() cycle (~50 ms).
```

**LDR calibration (called from `process_record_kb()` on `LDR_CAL_UP` / `LDR_CAL_DN` key-down):**

```
ldr_cal_adjust(int8_t delta):     // delta = +LDR_CAL_STEP or -LDR_CAL_STEP
  is_local = (record->event.key.row < MATRIX_ROWS / 2) == is_left_hand

  if is_local:
    ldr_cal_local = clamp(ldr_cal_local + delta, -LDR_CAL_MAX, +LDR_CAL_MAX)
    eeprom_write_byte(&ldr_cal_local_eeprom_addr, (uint8_t)ldr_cal_local)
    send SET_LDR_CAL(ldr_cal_local) to OLED on next oled_module_task() cycle
  else:
    new_offset = clamp(mercurio_state.ldr_cal_offset + delta, -LDR_CAL_MAX, +LDR_CAL_MAX)
    mercurio_state.ldr_cal_offset = new_offset   // propagates to Slave on next USART sync

// On Slave side, in housekeeping_task_slave():
  if mercurio_state.ldr_cal_offset != ldr_cal_local:
    ldr_cal_local = mercurio_state.ldr_cal_offset
    eeprom_write_byte(&ldr_cal_local_eeprom_addr, (uint8_t)ldr_cal_local)
```

> **EEPROM defaults:** On first boot (or after `EE_CLR`), `ldr_enabled` initializes to `true` (`LDR_DEFAULT_ENABLED`) and `ldr_cal_local` initializes to `0` on both halves.
> **When disabled:** `rgb_matrix_set_val()` is not called by `ldr_task()`; manual brightness (`RGB_VAI` / `RGB_VAD`) takes effect instead.
> **Calibration range:** `±LDR_CAL_MAX` (127) ADC units. With the useful ADC window of 80–900 (820 units), this allows shifting the effective dimming curve by up to ±15.5% to compensate LDR sensitivity differences between halves.

### E. Suggested Keymap Layouts

With 35 keys per half (70 total) across 5 rows × 7 columns, three layout philosophies are described below. All use the 4-layer structure from FR-16.

#### Option A — Standard QWERTY (recommended starting point)

```
Left half                                      Right half
,-----+-----+-----+-----+-----+-----+-----.   ,-----+-----+-----+-----+-----+-----+-----.
| Esc |  1  |  2  |  3  |  4  |  5  |  6  |   |  7  |  8  |  9  |  0  |  -  |  =  | Bsp |
+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+
| Tab |  Q  |  W  |  E  |  R  |  T  |  [  |   |  ]  |  Y  |  U  |  I  |  O  |  P  |  \  |
+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+
|LCAP |  A  |  S  |  D  |  F  |  G  |  `  |   |  '  |  H  |  J  |  K  |  L  |  ;  | Ent |
+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+
|LSft |  Z  |  X  |  C  |  V  |  B  | Del |   | PUp |  N  |  M  |  ,  |  .  |  /  |RSft |
+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+
|LCtl |LGui |LAlt |LOWER| Spc | Bsp | App |   | PDn | Spc |RAISE|RAlt |RGui | Ins |RCtl |
`-----+-----+-----+-----+-----+-----+-----'   `-----+-----+-----+-----+-----+-----+-----'
                         (OLED)                               (PSP joystick)
```

**LOWER layer (symbols):**

```
Left                                           Right
,-----+-----+-----+-----+-----+-----+-----.   ,-----+-----+-----+-----+-----+-----+-----.
|  ~  |  !  |  @  |  #  |  $  |  %  |  ^  |   |  &  |  *  |  (  |  )  |  _  |  +  | Del |
+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+
|     |     |     |     |     |     |  {  |   |  }  |     |     |     |     |     |  |  |
+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+
|     |     |     |     |     |     |     |   |     |     |     |     |     |  :  |     |
+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+
|     |     |     |     |     |     |     |   |     |     |     |  <  |  >  |  ?  |     |
+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+
|     |     |     |LOWER|     |     |     |   |     |     |RAISE|     |     |     |     |
`-----+-----+-----+-----+-----+-----+-----'   `-----+-----+-----+-----+-----+-----+-----'
```

**RAISE layer (navigation + F-keys):**

```
Left                                           Right
,-----+-----+-----+-----+-----+-----+-----.   ,-----+-----+-----+-----+-----+-----+-----.
|     | F1  | F2  | F3  | F4  | F5  | F6  |   | F7  | F8  | F9  | F10 | F11 | F12 |     |
+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+
|     |     | M_U |     |     |     |     |   |     | PgU | Up  | PgD |     |     |     |
+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+
|     | M_L | M_D | M_R |     |     |     |   |     |Left |Down |Right| End |     |     |
+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+
|     | M_B1| M_B2| M_B3|     |     |     |   |     |     |Home |     |     |     |     |
+-----+-----+-----+-----+-----+-----+-----+   +-----+-----+-----+-----+-----+-----+-----+
|     |     |     |LOWER|     |     |     |   |     |     |RAISE|     |     |     |     |
`-----+-----+-----+-----+-----+-----+-----'   `-----+-----+-----+-----+-----+-----+-----'
```

> M_U/M_L/M_D/M_R are `MOUSEKEY_ENABLE` cursor keys. Since the right half has a physical PSP joystick, these may be removed from RAISE if preferred.

#### Option B — Colemak-DH (reduced finger travel)

Drop-in replacement for Option A's BASE layer only. All other layers remain identical.

```
Left half (rows 1–3 only; rows 0 and 4 identical to Option A)
Row 1: Tab  Q    W    F    P    B    [
Row 2: Caps A    R    S    T    G    `
Row 3: LSft Z    X    C    D    V    Del

Right half (rows 1–3 only)
Row 1: ]    J    L    U    Y    ;    \
Row 2: '    M    N    E    I    O    Ent
Row 3: PUp  K    H    ,    .    /    RSft
```

#### Option C — Mod-Tap Thumb Cluster (advanced)

Keep QWERTY from Option A but convert Row 4 to Mod-Tap keys, freeing Caps Lock and dedicated modifier columns.

| Physical key | Mod-Tap assignment         | Tap →  | Hold →  |
| :----------- | :------------------------- | :----- | :------ |
| Row4 Col3 L  | `LT(_LOWER, KC_SPC)`       | Space  | LOWER   |
| Row4 Col4 L  | `MT(MOD_LCTL, KC_SPC)`     | Space  | LCtrl   |
| Row4 Col2 L  | `MT(MOD_LALT, KC_TAB)`     | Tab    | LAlt    |
| Row4 Col4 R  | `LT(_RAISE, KC_SPC)`       | Space  | RAISE   |
| Row4 Col3 R  | `MT(MOD_RCTL, KC_SPC)`     | Space  | RCtrl   |
| Row2 Col0 L  | `MT(MOD_LCTL, KC_CAPS)`    | Caps   | LCtrl   |

**Recommended `config.h` tuning for Mod-Tap:**

```c
#define TAPPING_TERM        175   // ms; lower = snappier, higher = more forgiving
#define TAPPING_TERM_PER_KEY      // enable per-key override via get_tapping_term()
#define PERMISSIVE_HOLD           // rolling keypresses register hold earlier
```

### F. Glossary

| Term              | Definition                                                                                          |
| :---------------- | :-------------------------------------------------------------------------------------------------- |
| V-USB             | Software USB implementation for AVR microcontrollers lacking hardware USB peripherals               |
| TRRS              | Tip-Ring-Ring-Sleeve 3.5 mm connector used for the inter-half cable                                |
| COL2ROW           | Diode orientation: cathode faces column pin; prevents key ghosting in matrix scanning               |
| Master            | The keyboard half currently connected to USB; owns V-USB stack and HID reporting                    |
| Slave             | The keyboard half not connected to USB; scans local matrix and forwards state via USART             |
| TWI               | Two-Wire Interface — AVR's hardware I2C peripheral                                                  |
| OC2               | Timer2 Output Compare pin (PD7) — toggles in hardware on timer compare match                       |
| CTC mode          | Clear Timer on Compare — AVR timer mode that auto-clears TCNT on compare match, generating periodic signals |
| SK6812MINI        | GRB addressable RGB LED (WS2812-compatible) in 3.5×3.5 mm package                                  |
| USBasp-loader     | V-USB based ISP bootloader for AVR; allows firmware flashing over USB without an external programmer |
| Bootmagic Lite    | QMK feature that enters the bootloader when a designated key is held at power-on                    |
| split_shared_data | QMK mechanism for synchronizing custom data structs from Master to Slave each transport cycle       |

---

## Revision History

| Rev | Date       | Author | Changes                                                                                                                                                                                         |
| :-- | :--------- | :----- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1.0 | 2026-03-23 | —      | Initial full specification                                                                                                                                                                      |
| 1.1 | 2026-03-24 | —      | Dynamic master via SPLIT_USB_DETECT; SPLIT_HAND_PIN (PD2); COL2ROW diode direction; USB on both halves; mouse 3-button hardware detail; corrected LDR divider circuit and ADC constants; Bootmagic section; Piezo driver section; USBasp/avrdude dev flash procedure |
| 1.2 | 2026-03-24 | —      | Reformatted to FSD structure: added Out of Scope, Functional Requirements (FR-01–FR-17), Non-Functional Requirements, Test Cases (TC-01–TC-16), Traceability Matrix, Risk Management, and Appendix |
| 1.3 | 2026-03-24 | —      | Added side note on QMK audio abstraction layer bridge possibility (§2.1) |
| 1.4 | 2026-03-24 | —      | LDR toggle feature: `LDR_TOG` on `_ADJUST` layer, EEPROM persistence, `ldr_enabled` in split shared state and OLED bulk payload; updated FR-09, FR-11, FR-17, TC-09, TC-09b, TC-10, traceability matrix, Appendix D, config.h |
| 1.5 | 2026-03-24 | —      | LDR per-half calibration: FR-18, `LDR_CAL_UP`/`LDR_CAL_DN` on `_ADJUST`, `ldr_cal_offset` in split shared state, `SET_LDR_CAL` OLED register, TC-09c, Appendix D calibration pseudocode, `LDR_CAL_STEP`/`LDR_CAL_MAX` in config.h |
