# sol/mercurio rev1-gamer — QMK build rules
#
# Left-hand half keyboard + PS/2 mouse (MX8731A)
# MCU: ATmega32A, 16 MHz external crystal, V-USB
#
# Phase 0 skeleton — all optional features disabled.
# Features are enabled incrementally per development phase.

# MCU / clock / bootloader
MCU        = atmega32a
F_CPU      = 16000000
BOOTLOADER = usbasploader

# Custom keyboard source (keyboard hooks)
# rev1_gamer.c is automatically included by QMK based on the keyboard name
SRC += ldr.c piezo.c piezo_effects.c i2c_module.c eeprom_ext.c
I2C_DRIVER_REQUIRED = yes

# Disable all unused features — P0 gate: flash < 5 KB
AUDIO_ENABLE     = no
BLUETOOTH_ENABLE = no
COMMAND_ENABLE   = no
CONSOLE_ENABLE   = no
NKRO_ENABLE      = no

# PS/2 Mouse (Custom Driver)
MOUSE_ENABLE           = yes
PS2_MOUSE_ENABLE       = no
PS2_ENABLE             = no
SRC += custom_mouse.c

# RGB Matrix — enable in P3
RGB_MATRIX_DRIVER = ws2812
WS2812_DRIVER = bitbang

KEYBOARD_SHARED_EP = yes
MOUSE_SHARED_EP = yes

# OLED Display — SSD1306 over I2C
OLED_ENABLE = yes
OLED_DRIVER = ssd1306
OLED_TRANSPORT = i2c

LTO_ENABLE = yes