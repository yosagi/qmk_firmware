/* Copyright 2021 sekigon-gonnoc
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

#include "tp1s6.h"

#include "i2c_master.h"
#include "pointing_device.h"
#include "debug.h"

#include "mtch6102.h"

void keyboard_post_init_kb() {
    debug_enable = true;
    debug_mouse  = true;
    debug_matrix = true;

    wait_ms(100);

    i2c_init();
    init_mtch6102();
    keyboard_post_init_user();
}

void matrix_scan_kb() {
    static int      cnt = 0;
    static uint16_t last_read_time;
    mtch6102_data_t mtch6102_data;
    report_mouse_t  mouse_rep = {0};
    bool            is_valid  = false;

    // read mtch6102 data every 15ms
    if (timer_elapsed(last_read_time) > 15) {
        last_read_time = timer_read();
        is_valid       = read_mtch6102(&mtch6102_data);
    }

    if (is_valid) {
        bool send_flag = process_mtch6102(&mtch6102_data, &mouse_rep);

        if (send_flag) {
            pointing_device_set_report(mouse_rep);
        }

        if (++cnt % 10 == 0) {
            if (debug_mouse) {
                dprintf("0x%02X 0x%02X %d %d\n", mtch6102_data.status, mtch6102_data.gesture, mtch6102_data.x, mtch6102_data.y);
            }
        }
    }

    matrix_scan_user();
}
