# MCU name
MCU = atmega32a
F_CPU = 16000000
# ARCH = AVR8
# OPT_DEFS += -DINTERRUPT_CONTROL_ENDPOINT
#OPT_DEFS += -DLEFT_HAND
OPT_DEFS += -DRIGHT_HAND

# Bootloader selection
BOOTLOADER = USBasp

LTO_ENABLE = yes

# Build Options
#   change yes to no to disable
#

SPLIT_KEYBOARD = yes
SPLIT_TRANSPORT = custom

BOOTMAGIC_ENABLE = yes     # Virtual DIP switch configuration
MOUSEKEY_ENABLE = yes       # Mouse keys
EXTRAKEY_ENABLE = yes       # Audio control and System control
CONSOLE_ENABLE = no         # Console for debug
COMMAND_ENABLE = no         # Commands for debug and configuration
# Do not enable SLEEP_LED_ENABLE. it uses the same timer as BACKLIGHT_ENABLE
SLEEP_LED_ENABLE = no       # Breathing sleep LED during USB suspend
# if this doesn't work, see here: https://github.com/tmk/tmk_keyboard/wiki/FAQ#nkro-doesnt-work
NKRO_ENABLE = no            # USB Nkey Rollover
BACKLIGHT_ENABLE = no       # Enable keyboard backlight functionality
RGBLIGHT_ENABLE = yes        # Enable keyboard RGB underglow
AUDIO_ENABLE = no           # Audio output

RAW_ENABLE = yes

#BLUETOOTH_ENABLE = no       # Enable Bluetooth
#BLUETOOTH = RN42

PS2_MOUSE_ENABLE = no
PS2_USE_INT = no

OLED_DRIVER_ENABLE = yes

SPACE_CADET_ENABLE = no
GRAVE_ESC_ENABLE = no
MAGIC_ENABLE = no
