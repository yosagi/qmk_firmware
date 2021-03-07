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
#include "keymap_jp.h"
#include "dynamic_keymap.h"

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

#ifdef OLED_DRIVER_ENABLE
#    include "rev1.h"
#    include "oled_driver.h"
#endif

#ifdef OLED_DRIVER_ENABLE
void oled_task_user(void) { render_logo(); }
#endif

