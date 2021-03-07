
#pragma once

#if defined(KEYBOARD_keyboard_quantizer_rev2) || defined(KEYBOARD_keyboard_quantizer_rev3)
#    define DYNAMIC_KEYMAP_EEPROM_MAX_ADDR 4096
#    define DYNAMIC_KEYMAP_LAYER_COUNT 6
#else
#    define DYNAMIC_KEYMAP_LAYER_COUNT 2
#endif

// Use fixed parser
#undef QUANTIZER_REPORT_PARSER
#define QUANTIZER_REPORT_PARSER REPORT_PARSER_FIXED

