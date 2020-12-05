# Bootloader selection
BOOTLOADER = atmel-dfu

# Use custom matrix
CUSTOM_MATRIX = lite
SRC += matrix.c uart.c

EEPROM_DRIVER = i2c

CONSOLE_ENABLE = no        # Console for debug
VIRTSER_ENABLE = yes
