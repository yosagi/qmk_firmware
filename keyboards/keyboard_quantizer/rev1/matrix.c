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

#include "quantum.h"
#include "matrix.h"
#include "uart.h"

enum {
    SLIP_END     = 0xC0,
    SLIP_ESC     = 0xDB,
    SLIP_ESC_END = 0xDC,
    SLIP_ESC_ESC = 0xDD,
};

enum {
    LEN_L,
    LEN_H,
    MSG_TYP,
    DEV_TYP,
    DEV_NUM,
    EP,
    VID_L,
    VID_H,
    PID_L,
    PID_H,
    REPORT_START,
} packet_index;

enum {
    CONNECTED     = 0x01,
    DISCONNECTED  = 0x02,
    ERROR         = 0x03,
    DEVICE_POLL   = 0x04,
    DEVICE_STRING = 0x05,
    DEVICE_INFO   = 0x06,
    HID_INFO      = 0x07,
    STARTUP       = 0x08,
} msg_type;

#define SERIAL_BUFFER_LEN 256

void matrix_init_custom(void) { uart_init(115200); }

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool matrix_has_changed = false;
    bool receive_complete   = false;

    static uint8_t  buf[SERIAL_BUFFER_LEN];  // serial buffer
    static uint16_t widx     = 0;            // write index
    static bool     escaped  = false;        // escape flag
    static bool     overflow = false;        // overflow flag
    static uint8_t  pre_keyreport[8];

    while (uart_available()) {
        uint8_t c = uart_getchar();

        if (c == SLIP_END) {
            // dprintf("Detect END signal\n");
            if (overflow) {
                overflow = false;
            } else {
                receive_complete = true;
            }
            break;
        } else if (c == SLIP_ESC) {
            escaped = true;
        } else if (widx < sizeof(buf)) {
            if (escaped) {
                if (c == SLIP_ESC_END) {
                    buf[widx] = SLIP_END;
                } else if (c == SLIP_ESC_ESC) {
                    buf[widx] = SLIP_ESC;
                } else {
                    buf[widx] = c;
                }
                escaped = false;
            } else {
                buf[widx] = c;
            }

            widx++;
            if (widx > sizeof(buf)) {
                dprintf("Buffer overflow\n");
                overflow = true;
                widx     = 0;
            }
        }
    }

    if (receive_complete) {
        do {
            uint16_t msg_len = buf[LEN_L] | ((uint16_t)buf[LEN_H] << 8);

            // dprintf("Packet received:%d, %d\n", widx, msg_len);

            // validate packet length
            if (widx < REPORT_START || msg_len != widx - REPORT_START) {
                matrix_has_changed = false;
                break;
            }

            switch (buf[MSG_TYP]) {
                case DISCONNECTED:
                    dprintf("Disconnected\n");
                    for (uint8_t rowIdx = 0; rowIdx < MATRIX_ROWS; rowIdx++) {
                        if (current_matrix[rowIdx] != 0) {
                            matrix_has_changed     = true;
                            current_matrix[rowIdx] = 0;
                        }
                        memset(pre_keyreport, 0, sizeof(pre_keyreport));
                    }
                    break;

                case DEVICE_POLL:
                    // dprintf("Report received\n");
                    if (msg_len == 8 && buf[DEV_TYP] == 6) {
                        // accept only bootmode keyboard packet

                        // dprintf("%d %d %d %d %d %d %d %d\n", buf[REPORT_START], buf[REPORT_START + 1], buf[REPORT_START + 2], buf[REPORT_START + 3], buf[REPORT_START + 4], buf[REPORT_START + 5], buf[REPORT_START + 6], buf[REPORT_START + 7]);

                        if (memcmp(pre_keyreport, &buf[REPORT_START], sizeof(pre_keyreport)) == 0) {
                            // no change
                            matrix_has_changed = false;
                            break;
                        } else {
                            matrix_has_changed = true;
                            memcpy(pre_keyreport, &buf[REPORT_START], sizeof(pre_keyreport));
                        }

                        // clear all bit
                        for (uint8_t rowIdx = 0; rowIdx < MATRIX_ROWS; rowIdx++) {
                            current_matrix[rowIdx] = 0;
                        }

                        // set bits
                        for (uint8_t keyIdx = 0; keyIdx < 6; keyIdx++) {
                            uint8_t key    = buf[REPORT_START + keyIdx + 2];
                            uint8_t rowIdx = key / (sizeof(matrix_row_t) * 8);
                            uint8_t colIdx = key - rowIdx * (sizeof(matrix_row_t) * 8);
                            current_matrix[rowIdx] |= (1 << colIdx);
                        }

                        // modifier bits
                        current_matrix[MATRIX_ROWS - 1] = buf[REPORT_START];
                    }
                    break;
            }

        } while (0);

        receive_complete = false;
        widx             = 0;
        escaped          = false;
        overflow         = false;
        memset(buf, 0, sizeof(buf));
    }

    return matrix_has_changed;
}
