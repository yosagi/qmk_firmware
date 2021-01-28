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

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = {{KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, LCTL(KC_C), LCTL(KC_V), MO(1)}},
    [1] = {{KC_ENT, KC_BSPC, KC_NO, LCTL(KC_Z), LCTL(KC_Y), _______, _______, _______}},
};
// clang-format on

static int16_t gesture_move_x = 0;
static int16_t gesture_move_y = 0;
static bool    gesture_wait   = false;

uint8_t recognize_gesture(int16_t x, int16_t y) {
    uint8_t gesture_id = 0;

    if (abs(x) + abs(y)  < GESTURE_MOVE_THRESHOLD) {
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
    if (keycode == MO(1)) {
        if (record->event.pressed) {
            gesture_move_x = 0;
            gesture_move_y = 0;
            gesture_wait   = true;
        } else {
            gesture_wait       = false;
            uint8_t gesture_id = recognize_gesture(gesture_move_x, gesture_move_y);
            process_gesture(gesture_id);
            dprintf("id:%d x:%d,y:%d\n", gesture_id, gesture_move_x, gesture_move_y);
        }

    }

    return true;
}

extern bool          mouse_send_flag;
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
    mouse_send_flag = true;
    report_mouse_t mouse = pointing_device_get_report();

    if (layer_state == 0) {
        mouse.buttons = report->button;
    } else {
        mouse.buttons = 0;
    }

    mouse.x += report->x;
    mouse.y += report->y;
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
