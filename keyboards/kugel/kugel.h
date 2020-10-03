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

#pragma once

#include "quantum.h"

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t surface;
    uint8_t motion_flag;
} trackball_info_t;

typedef enum {
    TRACKBALL_NONE,
    TRACKBALL_BTO,
} trackball_type_t;

const trackball_info_t* get_trackball_info(void);

/* This is a shortcut to help you visually see your layout.
 *
 * The first section contains all of the arguments representing the physical
 * layout of the board and position of the keys.
 *
 * The second converts the arguments into a two-dimensional array which
 * represents the switch matrix.
 */

// clang-format off

#define LAYOUT(\
    k13, k15, k2, k3, k31, k32, k17, k18, k45, k46, k33, k35, k36, \
    k14, k16, k1, k4, k30, k29, k20, k19, k47, k48, k34, k37, \
    k11, k9, k8, k5, k27, k25, k22, k21, k43, k41, k38, \
        k10, k6, k28, k26, k24, k23, k42 \
    ){ \
        {k1, k2, k3, k4, k5, k6, KC_NO, k8, k9, k10, k11, KC_NO, k13, k14, k15, k16}, \
        {k17, k18, k19, k20, k21, k22, k23, k24, k25, k26, k27, k28, k29, k30, k31, k32}, \
        {k33, k34, k35, k36, k37, k38, KC_NO, KC_NO, k41, k42, k43, KC_NO, k45, k46, k47, k48} \
    }

// clang-format on
