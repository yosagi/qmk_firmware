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

#include "rev3.h"
#include "quantum.h"
#include "virtser.h"
#include "uart.h"
#include "lufa.h"
#include "bootloader.h"
#include <util/delay.h>

extern bool                       ch559UpdateMode;
extern USB_ClassInfo_CDC_Device_t cdc_device;

static void bootloader_check(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo) {
    // jump to bootloader if virtser baudrate is 1200bps
    if (CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 1200 && (CDCInterfaceInfo->State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR) == 0) {
        bootloader_jump();
    }
}

void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo) { bootloader_check(CDCInterfaceInfo); }

void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo) { bootloader_check(CDCInterfaceInfo); }

void keyboard_post_init_kb_rev(void) { print_set_sendchar(virtser_send); }

void virtser_recv(const uint8_t ch) {
    if (!ch559UpdateMode && (cdc_device.State.LineEncoding.BaudRateBPS == 57600)) {
        ch559UpdateMode = true;
        print_set_sendchar(send_char);

        // send bootloader command
        uart_putchar('k');
        uart_putchar('b');
        uart_putchar('\n');
        _delay_ms(100);

        // reinitialize uart
        uart_init(57600);
        // send dummy byte
        uart_putchar(0);
    }

    if (ch559UpdateMode) {
        uart_putchar(ch);
    }
}

void matrix_scan_kb() {
    if (ch559UpdateMode) {
        while (uart_available()) {
            virtser_send(uart_getchar());
        }
    }

    matrix_scan_user();
}
