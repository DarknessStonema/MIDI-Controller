#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"   // TinyUSB

#define MIDI_UART_ID uart0
#define MIDI_TX_PIN 0
#define MIDI_RX_PIN 1

void midi_uart_init() {
    uart_init(MIDI_UART_ID, 31250);
    gpio_set_function(MIDI_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(MIDI_RX_PIN, GPIO_FUNC_UART);
}

void midi_uart_send(uint8_t b) {
    uart_putc_raw(MIDI_UART_ID, b);
}

void midi_usb_send(uint8_t status, uint8_t data1, uint8_t data2) {
    uint8_t msg[3] = { status, data1, data2 };
    tud_midi_stream_write(0, msg, 3);
}

int main() {
    stdio_init_all();
    midi_uart_init();
    tusb_init();

    while (1) {
        tud_task(); // TinyUSB background

        if (tud_mounted()) {
            // USB is active: bridge UART <-> USB

            // Forward UART IN -> USB OUT
            if (uart_is_readable(MIDI_UART_ID)) {
                uint8_t b = uart_getc(MIDI_UART_ID);
                // For simplicity, send single bytes (real code should buffer full messages)
                uint8_t msg[3] = { b, 0, 0 };
                tud_midi_stream_write(0, msg, 1);
            }

            // Forward USB IN -> UART OUT
            if (tud_midi_available()) {
                uint8_t packet[4];
                uint32_t count = tud_midi_packet_read(packet);
                for (uint32_t i = 1; i < count; i++) {
                    midi_uart_send(packet[i]);
                }
            }
        } else {
            // Only power, no USB host: run UART only
            if (uart_is_readable(MIDI_UART_ID)) {
                uint8_t b = uart_getc(MIDI_UART_ID);
                midi_uart_send(b); // simple echo test
            }
        }
    }
}
