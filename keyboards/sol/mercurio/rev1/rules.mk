# MCU / clock / bootloader
MCU        = atmega32a
F_CPU      = 12000000
BOOTLOADER = usbasploader

# Custom keyboard source (keyboard hooks + shared state)
SRC += mercurio.c

# Disable all unused features — P0 gate: flash < 5 KB
AUDIO_ENABLE     = no
BLUETOOTH_ENABLE = no
COMMAND_ENABLE   = no
CONSOLE_ENABLE   = no
NKRO_ENABLE      = no

# MOUSE_SHARED_EP = no   # uncomment in P8 when pointing device is enabled
