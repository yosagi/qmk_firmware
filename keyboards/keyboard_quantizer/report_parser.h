

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t bits[256 / 8];
} keyboard_parse_result_t;

typedef struct {
    uint16_t button;
    int16_t  x;
    int16_t  y;
    int16_t  v;
    int16_t  h;
    uint16_t undefined;
} mouse_parse_result_t;

bool parse_report(uint8_t interface, uint8_t const *report, uint8_t len);
