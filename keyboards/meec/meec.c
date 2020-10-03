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

#include "meec.h"

#include "ECmicro.h"
#include "matrix.h"
#include "debug.h"

#define LOW_THRETHOLD 150
#define HIGH_THRETHOLD 200

void keyboard_post_init_kb() {
    debug_enable = true;
    debug_matrix = true;
}

void matrix_init_custom(void) {
    ecmicro_config_t ecmicro_config = {.low_threthold = LOW_THRETHOLD, .high_threthold = HIGH_THRETHOLD};

    ecmicro_init(&ecmicro_config);
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool updated = ecmicro_matrix_scan(current_matrix);

    static int cnt = 0;
    if (cnt++ == 300) {
        cnt = 0;
        ecmicro_dprint_matrix();
        dprintf("\n");
    }

    return updated;
}
