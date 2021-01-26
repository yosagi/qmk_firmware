# MCU name
MCU = atmega32u4

# Bootloader selection
BOOTLOADER = caterina

# Build Options
#   change yes to no to disable
#
BOOTMAGIC_ENABLE = lite     # Virtual DIP switch configuration
MOUSEKEY_ENABLE = yes       # Mouse keys
EXTRAKEY_ENABLE = yes       # Audio control and System control
CONSOLE_ENABLE = yes        # Console for debug
POINTING_DEVICE_ENABLE = yes
OLED_DRIVER_ENABLE = no
COMMAND_ENABLE = no         # Commands for debug and configuration
# Do not enable SLEEP_LED_ENABLE. it uses the same timer as BACKLIGHT_ENABLE
SLEEP_LED_ENABLE = no       # Breathing sleep LED during USB suspend
# if this doesn't work, see here: https://github.com/tmk/tmk_keyboard/wiki/FAQ#nkro-doesnt-work
NKRO_ENABLE = no            # USB Nkey Rollover
BACKLIGHT_ENABLE = no       # Enable keyboard backlight functionality
RGBLIGHT_ENABLE = no        # Enable keyboard RGB underglow
BLUETOOTH_ENABLE = no       # Enable Bluetooth
AUDIO_ENABLE = no           # Audio output

DEFAULT_FOLDER = keyboard_quantizer/rev3

SRC += report_descriptor_parser.c report_parser.c

QUANTIZER_PARSER = $(strip $(PARSER))
ifeq ($(QUANTIZER_PARSER), default)
  OPT_DEFS += -DQUANTIZER_REPORT_PARSER=REPORT_PARSER_DEFAULT
endif
ifeq ($(QUANTIZER_PARSER), fixed)
  OPT_DEFS += -DQUANTIZER_REPORT_PARSER=REPORT_PARSER_FIXED
endif
ifeq ($(QUANTIZER_PARSER), user)
  OPT_DEFS += -DQUANTIZER_REPORT_PARSER=REPORT_PARSER_USER
endif
