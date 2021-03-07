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

#include "keyboard_quantizer.h"
#include "rev3.h"

#include "quantum.h"
#include "virtser.h"
#include "uart.h"
#include "sendchar.h"
#include "lufa.h"
#include "bootloader.h"
#include <util/delay.h>

extern bool                       ch559_update_mode;
extern USB_ClassInfo_CDC_Device_t cdc_device;
extern uint8_t                    device_cnt;
extern uint8_t                    hid_info_cnt;

__attribute__((weak)) void virtser_send(const uint8_t byte) {}

static void bootloader_check(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo) {
    // jump to bootloader if virtser baudrate is 1200bps
    if (CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 1200 && (CDCInterfaceInfo->State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR) == 0) {
        bootloader_jump();
    }
}

void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo) { bootloader_check(CDCInterfaceInfo); }

void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo) { bootloader_check(CDCInterfaceInfo); }

int8_t virtser_sendchar(uint8_t c) {
    virtser_send(c);
    return 0;
}

void keyboard_post_init_kb_rev(void) {
#ifdef VIRTSER_ENABLE
    print_set_sendchar(virtser_sendchar);
#endif
}

void send_reset_cmd(void) {
    // send reset command to ch559
    hid_info_cnt = 0;
    device_cnt   = 0;

    uart_putchar('\n');
    _delay_ms(10);
    uart_putchar('k');
    uart_putchar('r');
    uart_putchar('\n');
}

#ifdef CH559_BOOTLOADER_ENABLE
static void send_bootloader_cmd(void) {
    // send bootloader jump command to ch559
    hid_info_cnt = 0;
    device_cnt   = 0;

    uart_putchar('\n');
    _delay_ms(100);
    uart_putchar('k');
    uart_putchar('b');
    uart_putchar('\n');
}
#endif

#ifdef QUANTIZER_INDICATOR_ENABLE
// LED status sent by host
uint8_t indicator_led_cmd;

bool led_update_kb(led_t led_state) {
    indicator_led_cmd = led_state.raw;

    return true;
}

__attribute__((weak)) uint8_t update_indicator_led(void) {
    uint8_t layer_led     = (layer_state >> 1) & 0x07;
    uint8_t indicator_led = layer_led;

    uint16_t phase = timer_read() % 600;
    if (phase < 150) {
        indicator_led &= ~indicator_led_cmd;
    } else if (phase < 300) {
        indicator_led |= indicator_led_cmd;
    } else if (phase < 450) {
        indicator_led &= ~(indicator_led_cmd & layer_led);
    }

    return indicator_led;
}

static void blink_indicator_led(uint8_t led)
{
    uart_putchar('\n');
    _delay_us(50);
    uart_putchar(0x80 | (led & 0x07));
    _delay_us(50);
    uart_putchar('\n');
}
#endif


void process_char(const uint8_t ch) {
    virtser_send(ch);

    switch (ch) {
        case 'd':
            if (debug_enable) {
                println("\nDisable dprint");
                debug_enable = false;
            } else {
                debug_enable = true;
                debug_keyboard = true;
                println("\nEnable dprint");
            }
            break;

        case 's':
            xprintf("hid_info_cnt:%d\n", hid_info_cnt);
            break;

        case 'r':
            println("Reset CH559");
            send_reset_cmd();
            break;

        case 'b':
            bootloader_jump();
            break;

        default:
            break;
    }
}

void virtser_recv(const uint8_t ch) {
#ifdef CH559_BOOTLOADER_ENABLE
    if (!ch559_update_mode && (cdc_device.State.LineEncoding.BaudRateBPS == 57600)) {
        // enter ch559 update mode
        ch559_update_mode = true;

        // disable debug print through virtser
        print_set_sendchar(sendchar);

        // send bootloader command
        send_bootloader_cmd();

        // wait bootloader activation
        _delay_ms(100);

        // reinitialize uart
        uart_init(57600);

        // send dummy byte
        uart_putchar(0);
    }
#endif

    if (ch559_update_mode) {
        // pass through received virtser data to uart
        uart_putchar(ch);
    } else {
        // process received data for simple console
        process_char(ch);
    }
}

void matrix_scan_kb() {
    if (ch559_update_mode) {
        // pass through received uart data to virtser
        while (uart_available()) {
            virtser_send(uart_getchar());
        }
    } else {
#ifdef QUANTIZER_INDICATOR_ENABLE
        blink_indicator_led(update_indicator_led());
#endif

        matrix_scan_user();
    }
}

