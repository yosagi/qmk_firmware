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

#include "debug.h"
#include "pointing_device.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // clang-format off
    [0] = LAYOUT(
        KC_ESC, KC_Q, KC_W, KC_E, KC_R, KC_T, LALT(KC_GRV), KC_Y, KC_U, KC_I, KC_O, KC_P, KC_BSPC,
        LCTL_T(KC_TAB), KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_QUOT,
        KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, KC_DOT, LSFT_T(KC_SLSH),
        KC_LGUI, KC_LALT, LT(1, KC_SPC), KC_BTN1, KC_BTN2, LT(2, KC_ENT), LSFT_T(KC_DEL)
    ),

    [1] = LAYOUT(
        KC_GRV, KC_EXLM, KC_AT, KC_HASH, KC_DLR, KC_PERC, LALT(KC_GRV), KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, KC_BSPC,
        LCTL_T(KC_TAB),  KC_1, KC_2, KC_3, KC_4, KC_5, KC_LCBR, KC_MINS, KC_EQL, KC_RCBR, KC_COLN, KC_DQUO,
        KC_LSFT, KC_6, KC_7, KC_8, KC_9, KC_0, KC_UNDS, KC_PLUS, KC_LBRC, KC_RBRC, LSFT_T(KC_BSLS),
        KC_LGUI, KC_LALT, KC_TRNS, KC_BTN1, KC_BTN2, KC_TRNS, LSFT_T(KC_DEL)
    ),

    [2] = LAYOUT(
        KC_TILD, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, LALT(KC_GRV), KC_HOME, KC_PGUP, KC_PGDN, KC_END, KC_PAUS, KC_INS,
        LCTL_T(KC_TAB), KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_LEFT, KC_DOWN, KC_UP, KC_RGHT, KC_SCLN, KC_QUOT,
        KC_LSFT, KC_F11, KC_F12, KC_NO, KC_NO, KC_NO, KC_NO, KC_M, KC_NO, KC_DOT, LSFT_T(KC_BSLS),
        KC_LGUI, KC_LALT, KC_TRNS, KC_BTN1, KC_BTN2, KC_TRNS, LSFT_T(KC_DEL)
    ),
    // clang-format on
};

uint8_t tb_button    = 0;
uint8_t tb_scrl_flag = false;

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_BTN1 ... KC_BTN5:
            if (record->event.pressed) {
                tb_button |= (1 << (keycode - KC_BTN1));
            } else {
                tb_button &= ~(1 << (keycode - KC_BTN1));
            }
            return false;
            break;
    }

    return true;
}

report_mouse_t mouse_rep;
void           send_mouse(report_mouse_t *report);

static inline int sgn(int16_t x) {
    if (x > 0) {
        return 1;
    } else if (x < 0) {
        return -1;
    } else {
        return 0;
    }
}

void matrix_scan_user(void) {
    int16_t tb_div = 64;

    static int16_t          x_surplus, y_surplus;
    static uint32_t         cnt;
    const trackball_info_t *tb_info = get_trackball_info();
    bool                    btn_change;

    btn_change        = (mouse_rep.buttons != tb_button);
    mouse_rep.buttons = tb_button;

    if (layer_state == 0) {
        tb_scrl_flag = 0;
        tb_div       = 64;
    } else if (layer_state == 0b100) {
        tb_scrl_flag = 1;
        tb_div       = 64;
    } else if (layer_state == 0b10) {
        tb_scrl_flag = 0;
        tb_div       = 256;
    }

    if (!tb_scrl_flag) {
        mouse_rep.h = 0;
        mouse_rep.v = 0;
        mouse_rep.x = (tb_info->x + x_surplus) / tb_div;
        mouse_rep.y = (tb_info->y + y_surplus) / tb_div;

        if (tb_info->x > 400 || tb_info->x < -400) {
            mouse_rep.x *= 2;
        }

        if (tb_info->y > 400 || tb_info->y < -400) {
            mouse_rep.y *= 2;
        }

        x_surplus = (tb_info->x + x_surplus) % tb_div;
        y_surplus = (tb_info->y + y_surplus) % tb_div;
    } else {
        mouse_rep.h = 1 * sgn((tb_info->x + x_surplus) / tb_div);
        mouse_rep.v = -1 * sgn((tb_info->y + y_surplus) / tb_div);
        mouse_rep.x = 0;
        mouse_rep.y = 0;

        x_surplus = (tb_info->x + x_surplus) % tb_div;
        y_surplus = (tb_info->y + y_surplus) % tb_div;
    }

    if (++cnt % 10 == 0 || (tb_info->motion_flag & 0x80)) {
        if (debug_mouse) dprintf("%3d: %6d %6d %3d\n", tb_info->motion_flag, tb_info->x, tb_info->y, tb_info->surface);
    }

    if (btn_change || (tb_info->motion_flag & 0x80)) {
        pointing_device_set_report(mouse_rep);
    }
}
