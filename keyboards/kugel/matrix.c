/*
Copyright 2020 sekigon-gonnoc

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>

#include "kugel.h"

#include "spi_master.h"
#include "quantum.h"
#include "matrix.h"

#include "paw3204.h"

#include "report.h"

#define IO_RESET C6
#define IO_ROW D7
#define IO_INT B4
#define TB_POW E6
#define TB_INT B5
#define CS_PIN_IO F6

#define IO_NUM 3

const uint8_t ioexpander_addrs[IO_NUM]        = {0x00, 0x01, 0x02};
const uint8_t ioexpander_cs_pins[IO_NUM]      = {CS_PIN_IO, CS_PIN_IO, CS_PIN_IO};
static bool   ioexpander_init_flag            = false;
static bool   ioexpander_init_succeed[IO_NUM] = {false};

trackball_type_t trackball_init_flag = TRACKBALL_NONE;
trackball_info_t tb_info;

static void spim_start(uint8_t* p_snd, uint8_t snd_len, uint8_t* p_rcv, uint8_t rcv_len, uint8_t cs_pin) {
    // set CS pin, MSB first, MODE 0, DIV2 => 8MHz
    spi_start(cs_pin, false, 3, 16);

    for (int idx = 0; idx < rcv_len; idx++) {
        p_rcv[idx] = spi_write(p_snd[idx]);
    }

    spi_stop();
}

static inline void init_trackball(void) {
    // read trackball id
    init_paw3204();
    if (read_pid_paw3204() == 0x30) {
        trackball_init_flag = TRACKBALL_BTO;
        return;
    }
}

static inline void init_ioexp(void) {
    // set pull-up of io expanders
    {
        uint8_t mcp23s_snd[] = {0x40 | 0 | 0, 0x0C, 0xFF, 0xFF};
        uint8_t mcp23s_rcv[] = {0, 0, 0, 0};
        spim_start(mcp23s_snd, sizeof(mcp23s_snd), mcp23s_rcv, sizeof(mcp23s_rcv), CS_PIN_IO);
    }

    // enable interrupt for all pins
    {
        uint8_t mcp23s_snd[] = {0x40 | 0 | 0, 0x04, 0xFF, 0xFF};
        uint8_t mcp23s_rcv[] = {0, 0, 0, 0};
        spim_start(mcp23s_snd, sizeof(mcp23s_snd), mcp23s_rcv, sizeof(mcp23s_rcv), CS_PIN_IO);
    }

    // set addressing mode of io expanders, INT pin mirror, and INT pin open
    // drain
    {
        uint8_t mcp23s_snd[] = {0x40 | 0 | 0, 0x0A, (1 << 6) | (1 << 3) | (1 << 2)};
        uint8_t mcp23s_rcv[] = {0, 0, 0};
        spim_start(mcp23s_snd, sizeof(mcp23s_snd), mcp23s_rcv, sizeof(mcp23s_rcv), CS_PIN_IO);

        // read the register of each IC and check initialization results
        for (int idx = 0; idx < IO_NUM; idx++) {
            mcp23s_snd[0] = 0x40 | (ioexpander_addrs[idx] << 1) | 1;
            spim_start(mcp23s_snd, sizeof(mcp23s_snd), mcp23s_rcv, sizeof(mcp23s_rcv), CS_PIN_IO);

            if (mcp23s_rcv[2] == mcp23s_snd[2]) {
                ioexpander_init_succeed[idx] = true;
            }
        }
    }

    ioexpander_init_flag = true;
    for (int idx = 0; idx < IO_NUM; idx++) {
        ioexpander_init_flag &= ioexpander_init_succeed[idx];
    }
}

static inline void init_pins(void) {
    // reset io expanders
    setPinOutput(IO_RESET);
    writePinLow(IO_RESET);
    setPinOutput(IO_ROW);
    writePinLow(IO_ROW);
    setPinInputHigh(IO_INT);

    writePinHigh(CS_PIN_IO);
    setPinOutput(CS_PIN_IO);

    // turn off trackball
    setPinOutput(TB_POW);
    writePinHigh(TB_POW);

    // turn on trackball
    writePinLow(TB_POW);

    // release reset of io expanders
    writePinHigh(IO_RESET);
}

void matrix_init_custom(void) {
    init_pins();
    spi_init();

    wait_ms(10);

    init_ioexp();

    wait_ms(10);

    init_trackball();

    return;
}

static inline void update_trackball_info(void) {
    tb_info.x = 0;
    tb_info.y = 0;

    if (trackball_init_flag == TRACKBALL_BTO) {
        // trackball communication packet
        // static uint32_t last_read_time;
        // static uint32_t last_active_time;
        //
        // if (timer_elapsed32(last_active_time) < 200 || timer_elapsed32(last_read_time) > 50) {
            uint8_t stat = 0;
            int8_t  x = 0, y = 0;

            read_paw3204(&stat, &x, &y);
            // last_read_time = timer_read32();

            tb_info.x           = -y * 16;
            tb_info.y           = x * 16;
            tb_info.surface     = 0;
            tb_info.motion_flag = stat;

        //     if (stat & 0x80) {
        //         last_active_time = timer_read32();
        //     }
        // } else {
        //     tb_info.x           = 0;
        //     tb_info.y           = 0;
        //     tb_info.motion_flag = 0;
        // }
    }
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool res = false;

    update_trackball_info();

    if (ioexpander_init_flag == false) {
        return false;
    }

    // skip if no pin is changed
    // if (readPin(IO_INT)) {
    //     return false;
    // }

    matrix_row_t row_state;
    uint8_t      spi_send[4];
    uint8_t      spi_recv[4];

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        spi_send[0] = 0x40 | (ioexpander_addrs[row] << 1) | 1;
        spi_send[1] = 0x12;
        spi_send[2] = 0;
        spi_send[3] = 0;
        spi_recv[0] = 0xFF;
        spi_recv[1] = 0xFF;
        spi_recv[2] = 0xFF;
        spi_recv[3] = 0xFF;

        spim_start(spi_send, sizeof(spi_send), spi_recv, sizeof(spi_recv), ioexpander_cs_pins[row]);

        row_state = ~((((uint16_t)spi_recv[3]) << 8) | spi_recv[2]);

        if (row_state != current_matrix[row]) {
            res = true;
        }
        current_matrix[row] = row_state;
    }

    return res;
}

const trackball_info_t* get_trackball_info(void) { return &tb_info; }
