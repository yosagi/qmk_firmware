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

#include "ECmicro.h"

#include "quantum.h"
#include "analog.h"
#include "debug.h"

// sensing channel definitions
#define S0 0
#define S1 1
#define S2 2
#define S3 3
#define S4 4
#define S5 5
#define S6 6
#define S7 7

// analog connction settings
#define DISCHARGE_PIN F6
#define ANALOG_PORT F7

// pin connections
const uint8_t row_pins[]     = MATRIX_ROW_PINS;
const uint8_t col_channels[] = MATRIX_COL_PINS;

static ecmicro_config_t config;
static uint16_t         ecmicro_sw_value[MATRIX_ROWS][MATRIX_COLS];

static inline void discharge_capacitor(void) { setPinOutput(DISCHARGE_PIN); }
static inline void charge_capacitor(uint8_t row) {
    setPinInput(DISCHARGE_PIN);
    writePinHigh(row_pins[row]);
}

static inline void clear_all_row_pins(void) {
    for (int row = 0; row < sizeof(row_pins); row++) {
        writePinLow(row_pins[row]);
    }
}

void init_mux_sel(void) {
    setPinOutput(F0);
    setPinOutput(F1);
    setPinOutput(F4);
}

void select_mux(uint8_t col) {
    uint8_t ch = col_channels[col];
    writePin(F0, ch & 1);
    writePin(F1, ch & 2);
    writePin(F4, ch & 4);
}

void init_row(void) {
    for (int idx = 0; idx < MATRIX_ROWS; idx++) {
        setPinOutput(row_pins[idx]);
        writePinLow(row_pins[idx]);
    }
}

int ecmicro_init(ecmicro_config_t const* const ecmicro_config) {
    // save config
    config = *ecmicro_config;

    // initialize discharge pin as discharge mode
    writePinLow(DISCHARGE_PIN);
    setPinOutput(DISCHARGE_PIN);

    // set analog reference
    analogReference(ADC_REF_POWER);

    // initialize drive lines
    init_row();

    // initialize multiplexer select pin
    init_mux_sel();

    // set discharge pin to charge mode
    setPinInput(DISCHARGE_PIN);

    return 0;
}

uint16_t ecmicro_readkey_raw(uint8_t row, uint8_t col) {
    uint16_t sw_value = 0;

    discharge_capacitor();

    select_mux(col);

    clear_all_row_pins();

    cli();

    charge_capacitor(row);

    sw_value = analogReadPin(ANALOG_PORT);

    sei();

    return sw_value;
}

bool ecmicro_update_key(matrix_row_t* current_row, uint8_t col, uint16_t sw_value) {
    bool current_state = (*current_row >> col) & 1;

    // press to release
    if (current_state && sw_value < config.low_threthold) {
        *current_row &= ~(1 << col);
        return true;
    }

    // release to press
    if ((!current_state) && sw_value > config.high_threthold) {
        *current_row |= (1 << col);
        return true;
    }

    return false;
}

bool ecmicro_matrix_scan(matrix_row_t current_matrix[]) {
    bool updated = false;

    for (int col = 0; col < MATRIX_COLS; col++) {
        for (int row = 0; row < MATRIX_ROWS; row++) {
            ecmicro_sw_value[row][col] = ecmicro_readkey_raw(row, col);
            updated |= ecmicro_update_key(&current_matrix[row], col, ecmicro_sw_value[row][col]);
        }
    }

    return updated;
}

void ecmicro_dprint_matrix(void) {
    for (int row = 0; row < MATRIX_ROWS; row++) {
        for (int col = 0; col < MATRIX_COLS; col++) {
            dprintf("%5d", ecmicro_sw_value[row][col]);
        }
        dprintf("\n");
    }
}
