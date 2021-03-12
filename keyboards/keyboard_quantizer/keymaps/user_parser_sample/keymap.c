/* Copyright 2020 sekigon-gonnoc
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H
#include <string.h>
#include "keymap_jp.h"
#include "dynamic_keymap.h"
#include "keyboard_quantizer.h"
#include "report_parser.h"

static uint8_t pre_keyreport[8];
// clang-format off
// Set empty to reduce firm size
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = { };
// clang-format on

// Override to avoid messy initial keymap show on VIA
// Patch to dynamic_keymap.c is required
void dynamic_keymap_reset(void) {
    for (int layer = 0; layer < DYNAMIC_KEYMAP_LAYER_COUNT; layer++) {
        for (int row = 0; row < MATRIX_ROWS - 1; row++) {
            for (int column = 0; column < MATRIX_COLS; column++) {
                dynamic_keymap_set_keycode(layer, row, column, row * MATRIX_COLS + column);
            }
        }
        for (int column = 0; column < MATRIX_COLS; column++) {
            dynamic_keymap_set_keycode(layer, MATRIX_ROWS - 1, column, KC_LCTRL + column);
        }
    }
}

void report_descriptor_parser_user(uint8_t interface, uint8_t const* desc, uint16_t len) {}

void on_disconnect_device_user(uint8_t device) {
    memset(pre_keyreport, 0, sizeof(pre_keyreport));
}

bool thinkpad_trakpoint_keyboard_parser(uint8_t const* buf, uint16_t len, matrix_row_t* current_matrix) {
    uint16_t msg_len = buf[LEN_L] | ((uint16_t)buf[LEN_H] << 8);
    if (buf[DEV_NUM] == 0) {
        // Interface 0 is keyboard report
        return report_parser_fixed(buf, msg_len, pre_keyreport, current_matrix);
    } else if (buf[DEV_NUM] == 1) {
        // Interface 1 is trackpoint report
        if (buf[REPORT_START] == 1) {
            // Report ID1 is mouse report
            mouse_parse_result_t mouse = {0};
            mouse.button               = buf[REPORT_START + 1];
            mouse.x                    = buf[REPORT_START + 2];
            mouse.y                    = buf[REPORT_START + 3];
            mouse.v                    = buf[REPORT_START + 4];
            // hscroll is send by ID 0x16
            // mouse.h                    = buf[REPORT_START + 5];
            mouse_report_hook(&mouse);
        } else if (buf[REPORT_START] == 0x16) {
            // Report ID22 is Hscroll report
            mouse_parse_result_t mouse = {0};
            mouse.h                    = buf[REPORT_START + 1];
            mouse_report_hook(&mouse);
        }
    }

    return false;
}

bool report_parser_user(uint8_t const* buf, uint16_t len, matrix_row_t* current_matrix) {
    if (buf[VID_H] == 0x17 && buf[VID_L] == 0xef && buf[PID_H] == 0x60 && buf[PID_L] == 0x47) {
        return thinkpad_trakpoint_keyboard_parser(buf, len, current_matrix);
    }

    else {
        uint16_t msg_len = buf[LEN_L] | ((uint16_t)buf[LEN_H] << 8);
        return report_parser_fixed(buf, msg_len, pre_keyreport, current_matrix);
    }

    return false;
}
