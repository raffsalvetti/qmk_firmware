#include "piezo.h"
#include <avr/io.h>
#include "quantum.h"

static uint16_t current_duration = 0;
static uint32_t piezo_timer = 0;

void piezo_init(void) {
    // PD7 (OC2) as output
    DDRD |= (1 << 7);
    
    // Timer2 CTC mode, Toggle OC2 on compare match
    // WGM21=1, WGM20=0, COM21=0, COM20=1
    TCCR2 = (1 << WGM21) | (1 << COM20);
}

void piezo_task(void) {
    if (current_duration > 0) {
        if (timer_elapsed32(piezo_timer) < current_duration) 
            return;
        piezo_stop();
    }
}

void piezo_play(uint8_t ocr2_val, uint16_t duration_ms) {
    OCR2 = ocr2_val;
    TCNT2 = 0;
    current_duration = duration_ms;
    piezo_timer = timer_read32();
    
    // Start timer with prescaler /64 (CS22=1)
    TCCR2 = (TCCR2 & ~((1 << CS22) | (1 << CS21) | (1 << CS20))) | (1 << CS22);
}

void piezo_stop(void) {
    TCCR2 &= ~((1 << CS22) | (1 << CS21) | (1 << CS20));
    current_duration = 0;
}
