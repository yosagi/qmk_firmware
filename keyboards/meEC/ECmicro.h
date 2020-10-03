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

#include <stdint.h>
#include <stdbool.h>

#include "matrix.h"

typedef struct {
    uint16_t low_threthold;
    uint16_t high_threthold;
} ecmicro_config_t;

int      ecmicro_init(ecmicro_config_t const* const ecmicro_config);
bool     ecmicro_matrix_scan(matrix_row_t current_matrix[]);
void     ecmicro_dprint_matrix(void);
uint16_t ecmicro_readkey_raw(uint8_t row, uint8_t col);
bool     ecmicro_update_key(matrix_row_t* current_row, uint8_t col, uint16_t sw_value);
