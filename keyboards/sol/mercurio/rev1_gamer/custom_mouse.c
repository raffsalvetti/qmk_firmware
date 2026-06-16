#include <stdbool.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "quantum.h"
#include "host.h"
#include "report.h"
#include "print.h"
#include "config.h"

// Basic PS/2 Commands
#define PS2_MOUSE_RESET                  0xFF
#define PS2_MOUSE_ENABLE_DATA_REPORTING  0xF4
#define PS2_MOUSE_SET_STREAM_MODE        0xEA

// ATmega32A USART Hardware definitions
#define PS2_CLOCK_BIT (PS2_CLOCK_PIN & 0xF)
#define PS2_DATA_BIT  (PS2_DATA_PIN & 0xF)

// Ring buffer to store scan codes from mouse
#define PBUF_SIZE 32
static uint8_t pbuf[PBUF_SIZE];
static volatile uint8_t pbuf_head = 0;
static volatile uint8_t pbuf_tail = 0;

static inline void pbuf_enqueue(uint8_t data) {
    uint8_t sreg = SREG;
    cli();
    uint8_t next = (pbuf_head + 1) % PBUF_SIZE;
    if (next != pbuf_tail) {
        pbuf[pbuf_head] = data;
        pbuf_head       = next;
    }
    SREG = sreg;
}

static inline bool pbuf_has_data(void) {
    uint8_t sreg = SREG;
    cli();
    bool hasData = (pbuf_head != pbuf_tail);
    SREG = sreg;
    return hasData;
}

static inline uint8_t pbuf_dequeue(void) {
    uint8_t sreg = SREG;
    cli();
    uint8_t val = 0;
    if (pbuf_head != pbuf_tail) {
        val = pbuf[pbuf_tail];
        pbuf_tail = (pbuf_tail + 1) % PBUF_SIZE;
    }
    SREG = sreg;
    return val;
}

// Pin control
void clock_hi(void) {
    gpio_set_pin_input_high(PS2_CLOCK_PIN);
}
void clock_lo(void) {
    gpio_set_pin_output(PS2_CLOCK_PIN);
    gpio_write_pin_low(PS2_CLOCK_PIN);
}
bool clock_in(void) {
    return gpio_read_pin(PS2_CLOCK_PIN);
}
void data_hi(void) {
    gpio_set_pin_input_high(PS2_DATA_PIN);
}
void data_lo(void) {
    gpio_set_pin_output(PS2_DATA_PIN);
    gpio_write_pin_low(PS2_DATA_PIN);
}
bool data_in(void) {
    return gpio_read_pin(PS2_DATA_PIN);
}

// Wait macros for bit-banging
static inline uint16_t wait_clock_lo(uint16_t us) {
    while (clock_in() && us) {
        asm(""); _delay_us(1); us--;
    }
    return us;
}
static inline uint16_t wait_clock_hi(uint16_t us) {
    while (!clock_in() && us) {
        asm(""); _delay_us(1); us--;
    }
    return us;
}
static inline uint16_t wait_data_lo(uint16_t us) {
    while (data_in() && us) {
        asm(""); _delay_us(1); us--;
    }
    return us;
}
static inline uint16_t wait_data_hi(uint16_t us) {
    while (!data_in() && us) {
        asm(""); _delay_us(1); us--;
    }
    return us;
}

#define WAIT(stat, us)          \
    do {                        \
        if (!wait_##stat(us)) { \
            goto ERROR;         \
        }                       \
    } while (0)

// USART Initialization
static void usart_init(void) {
    UBRRH = (uint8_t)(103 >> 8);
    UBRRL = (uint8_t)(103);
    UCSRC = (1 << URSEL) | (1 << UMSEL) | (1 << UCSZ1) | (1 << UCSZ0) | (1 << UCPOL);
}
static void usart_rx_int_on(void) {
    UCSRB = (1 << RXCIE) | (1 << RXEN);
}
static void usart_off(void) {
    UCSRC = (1 << URSEL);
    UCSRB &= ~((1 << RXEN) | (1 << TXEN));
}

// Removed ISR_NOBLOCK! Nested interrupts on a noisy line can cause a Stack Overflow!
// This ISR is so fast (15 cycles) that it doesn't need to be non-blocking. V-USB is safe.
ISR(USART_RXC_vect) {
    uint8_t error = UCSRA & ((1 << FE) | (1 << DOR) | (1 << UPE));
    uint8_t data  = UDR;
    if (!error) {
        pbuf_enqueue(data);
    }
}

void custom_mouse_init(void) {
    clock_hi();
    data_hi();
    usart_init();
    // Do NOT enable RX interrupts yet!
    // V-USB enumeration must finish first. We will enable it 8.5s later.
}

static void ps2_host_send_real(uint8_t data) {
    bool parity = true;

    usart_off();
    _delay_us(100);

    /* 'Request to Send' and Start bit */
    data_lo();
    clock_hi();
    WAIT(clock_lo, 10000);

    /* Data bit[2-9] */
    for (uint8_t i = 0; i < 8; i++) {
        _delay_us(15);
        if (data & (1 << i)) {
            parity = !parity;
            data_hi();
        } else {
            data_lo();
        }
        WAIT(clock_hi, 50);
        WAIT(clock_lo, 50);
    }

    /* Parity bit */
    _delay_us(15);
    if (parity) {
        data_hi();
    } else {
        data_lo();
    }
    WAIT(clock_hi, 50);
    WAIT(clock_lo, 50);

    /* Stop bit */
    _delay_us(15);
    data_hi();

    /* Ack */
    WAIT(data_lo, 50);
    WAIT(clock_lo, 50);

    /* wait for idle state */
    WAIT(clock_hi, 50);
    WAIT(data_hi, 50);

    usart_init();
    usart_rx_int_on();
    return;

ERROR:
    usart_init();
    usart_rx_int_on();
    return;
}

// Track async initialization state
static bool ps2_vendor_inited = false;

// State Machine for asynchronous initialization and parsing
void custom_mouse_task(void) {
    static uint8_t state = 0;
    static uint32_t init_timer = 0;

    if (init_timer == 0) {
        init_timer = timer_read32();
    }

    uint32_t elapsed = timer_elapsed32(init_timer);

    if (!ps2_vendor_inited) {
        // Wait 8500ms after boot (waiting for the 8s boot screen to disappear), then send RESET
        if (state == 0 && elapsed > 8500) {
            // V-USB is 100% enumerated. Safe to turn on USART RX interrupts!
            usart_rx_int_on();
            ps2_host_send_real(PS2_MOUSE_RESET);
            state = 1;
        }
        // Wait 500ms for BAT and DevID. Drain stale bytes, then send ENABLE.
        else if (state == 1 && elapsed > 9000) {
            while (pbuf_has_data()) pbuf_dequeue();
            ps2_host_send_real(PS2_MOUSE_ENABLE_DATA_REPORTING);
            state = 2;
        }
        // Wait 100ms, then send STREAM MODE
        else if (state == 2 && elapsed > 9100) {
            ps2_host_send_real(PS2_MOUSE_SET_STREAM_MODE);
            state = 3;
        }
        // Wait 100ms for final ACK. Drain again and finish initialization!
        else if (state == 3 && elapsed > 9200) {
            while (pbuf_has_data()) pbuf_dequeue();
            ps2_vendor_inited = true;
        }
        return; // Do not process data until initialized
    }

    // Process real mouse data
    static uint8_t mouse_packet[3];
    static uint8_t mouse_byte_count = 0;

    while (pbuf_has_data()) {
        uint8_t byte = pbuf_dequeue();

        // Byte 0 MUST have bit 3 set (sync bit). If it doesn't, we're out of sync.
        if (mouse_byte_count == 0 && !(byte & (1 << 3))) {
            continue; // Discard garbage byte and keep looking for a valid start byte
        }

        mouse_packet[mouse_byte_count++] = byte;

        if (mouse_byte_count == 3) {
            mouse_byte_count = 0; // Ready for next packet

            uint8_t b0 = mouse_packet[0];
            uint8_t b1 = mouse_packet[1];
            uint8_t b2 = mouse_packet[2];

            // Final sanity check: bit 3 must be 1. (Overflow bits 6/7 are usually 0 but can be 1 on fast flicks)
            if (!(b0 & (1 << 3))) {
                continue; // Packet somehow got corrupted, drop it
            }

            // We have a full, valid 3-byte packet! Parse and inject it.
            report_mouse_t report;
            memset(&report, 0, sizeof(report));

            // Buttons
            if (b0 & (1 << 0)) report.buttons |= MOUSE_BTN1; // Left
            if (b0 & (1 << 1)) report.buttons |= MOUSE_BTN2; // Right
            if (b0 & (1 << 2)) report.buttons |= MOUSE_BTN3; // Middle

            // X Movement (Sign extend if necessary)
            int16_t x = b1;
            if (b0 & (1 << 4)) x |= 0xFF00; // X Sign Bit
            // Optional: Handle X Overflow
            if (b0 & (1 << 6)) x = (b0 & (1 << 4)) ? -255 : 255;
            report.x = x;

            // Y Movement (PS/2 Y is inverted from USB HID Y)
            int16_t y = b2;
            if (b0 & (1 << 5)) y |= 0xFF00; // Y Sign Bit
            // Optional: Handle Y Overflow
            if (b0 & (1 << 7)) y = (b0 & (1 << 5)) ? -255 : 255;
            report.y = -y; // Invert Y for USB HID

            // Send to host!
            host_mouse_send(&report);
        }
    }
}
