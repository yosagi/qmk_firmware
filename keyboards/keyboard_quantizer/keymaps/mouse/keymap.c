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

#include "pointing_device.h"
#include "debug.h"

#include "report_parser.h"

#define GESTURE_MOVE_THRESHOLD 90

enum custom_keycodes {
    SPD_1 = SAFE_RANGE,
    SPD_2,
    SPD_3,
};

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = {{KC_BTN1, KC_BTN2, KC_BTN3, KC_BTN4, KC_BTN5, LCTL(KC_C), LCTL(KC_V), MO(1)}},
    [1] = {{KC_ENT, KC_BSPC, SPD_3, LCTL(KC_Z), LCTL(KC_Y), SPD_1, SPD_2, _______}},
};
// clang-format on

extern bool mouse_send_flag;

static uint8_t spd_rate_num   = 1;
static uint8_t spd_rate_den   = 1;
static int16_t gesture_move_x = 0;
static int16_t gesture_move_y = 0;
static bool    gesture_wait   = false;
static uint8_t kc_no_to_kc_offset = 0;
static uint8_t btn_release_flag = 0;

uint8_t recognize_gesture(int16_t x, int16_t y) {
    uint8_t gesture_id = 0;

    if (abs(x) + abs(y) < GESTURE_MOVE_THRESHOLD) {
        gesture_id = 0;
    } else if (x >= 0 && y >= 0) {
        gesture_id = 1;
    } else if (x < 0 && y >= 0) {
        gesture_id = 2;
    } else if (x < 0 && y < 0) {
        gesture_id = 3;
    } else if (x >= 0 && y < 0) {
        gesture_id = 4;
    }

    return gesture_id;
}

void process_gesture(uint8_t gesture_id) {
    switch (gesture_id) {
        case 1:
            tap_code(KC_PGUP);
            break;
        case 2:
            tap_code(KC_PGDN);
            break;
        case 3:
            tap_code(KC_HOME);
            break;
        case 4:
            tap_code(KC_END);
            break;
        default:
            break;
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t* record) {
    switch (keycode) {
        case KC_BTN1 ... KC_BTN5: {
            report_mouse_t mouse = pointing_device_get_report();
            if (record->event.pressed) {
                mouse.buttons |= (1 << (keycode - KC_BTN1));
            } else {
                mouse.buttons &= ~(1 << (keycode - KC_BTN1));
            }
            pointing_device_set_report(mouse);

            return false;
        } break;

        case SPD_1:
            spd_rate_num = 1;
            spd_rate_den = 1;
            return false;

        case SPD_2:
            spd_rate_num = 1;
            spd_rate_den = 2;
            return false;

        case SPD_3:
            spd_rate_num = 2;
            spd_rate_den = 1;
            return false;

        default:
            break;
    }

    // Enable layer tap with KC_BTNx
    if (keycode >= QK_LAYER_TAP && keycode <= QK_LAYER_TAP_MAX) {
        uint8_t kc = keycode & 0xFF;
        if (kc == KC_NO) {
            kc = kc_no_to_kc_offset;
            dprintf("KC:%d, tap:%d\n", kc, record->tap.count);
            if (record->tap.count > 0 && !record->tap.interrupted) {
                // set mouse button bit
                report_mouse_t mouse = pointing_device_get_report();
                mouse.buttons |= (1 << (kc - KC_BTN1));
                pointing_device_set_report(mouse);
                mouse_send_flag = true;
                btn_release_flag |= (1 << (kc - KC_BTN1));
            }
        }
    }

    return true;
}

// override keymap_key_to_keycode
uint16_t keymap_key_to_keycode(uint8_t layer, keypos_t key) {
    uint16_t keycode = pgm_read_word(&keymaps[(layer)][(key.row)][(key.col)]);

    if (keycode >= QK_LAYER_TAP && keycode <= QK_LAYER_TAP_MAX) {
        uint8_t kc = keycode & 0xFF;
        if (kc >= KC_BTN1 && kc <= KC_BTN5) {
            kc_no_to_kc_offset = kc;
            return keycode & 0xFF00;
        } else {
            kc_no_to_kc_offset = 0;
        }
    }

    return keycode;
}

void matrix_scan_user(void) {
    if (btn_release_flag)
    {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.buttons &= ~btn_release_flag;
        btn_release_flag = 0;
        pointing_device_set_report(mouse);
        mouse_send_flag = true;
    }
}

void post_process_record_user(uint16_t keycode, keyrecord_t* record) {
    if (layer_state_is(1) && gesture_wait == false) {
        gesture_wait   = true;
        gesture_move_x = 0;
        gesture_move_y = 0;
    } else if (gesture_wait == true && layer_state_is(0)) {
        gesture_wait       = false;
        uint8_t gesture_id = recognize_gesture(gesture_move_x, gesture_move_y);
        process_gesture(gesture_id);
        dprintf("id:%d x:%d,y:%d\n", gesture_id, gesture_move_x, gesture_move_y);
    }
}

extern bool          matrix_has_changed;
extern matrix_row_t* matrix_dest;

void mouse_report_hook(mouse_parse_result_t const* report) {
    if (debug_enable) {
        xprintf("Mouse report\n");
        xprintf("b:%d ", report->button);
        xprintf("x:%d ", report->x);
        xprintf("y:%d ", report->y);
        xprintf("v:%d ", report->v);
        xprintf("h:%d ", report->h);
        xprintf("undef:%u\n", report->undefined);
    }

    //
    // Assign buttons to matrix
    // 8 button mouse is assumed
    //
    uint8_t button_prev    = matrix_dest[0];
    uint8_t button_current = (report->button & 0x1F) | ((report->undefined & 0x7) << 5);

    if (button_current != button_prev) {
        matrix_has_changed = true;
    }
    matrix_dest[0] = button_current;

    //
    // Assign mouse movement
    //
    mouse_send_flag      = true;
    report_mouse_t mouse = pointing_device_get_report();

    if (layer_state != 0) {
        mouse.buttons = 0;
    }

    static int16_t x_rem;
    static int16_t y_rem;

    int16_t x = (x_rem + report->x) * spd_rate_num / spd_rate_den;
    int16_t y = (y_rem + report->y) * spd_rate_num / spd_rate_den;

    if (spd_rate_den - spd_rate_num > 0) {
        x_rem = (x_rem + report->x) - (x * spd_rate_den);
        y_rem = (y_rem + report->y) - (y * spd_rate_den);
    } else {
        x_rem = 0;
        y_rem = 0;
    }

    mouse.x += x;
    mouse.y += y;
    mouse.v += report->v;
    mouse.h += report->h;

    pointing_device_set_report(mouse);

    //
    // Save movement to recognize gesture
    //
    if (gesture_wait) {
        gesture_move_x += report->x;
        gesture_move_y += report->y;
    }
}
