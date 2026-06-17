#include "ldr.h"
#include "quantum.h"
#include <avr/io.h>
#include <stdlib.h>

bool ldr_enabled = LDR_DEFAULT_ENABLED;
uint16_t ldr_adc_val = 0;
static uint32_t last_ldr_poll = 0;

void ldr_init(void) {
    // ADC channel 6 (PA6)
    // AVCC reference (REFS0 = 1)
    ADMUX = (1 << REFS0) | 6; 
    
    // Enable ADC (ADEN)
    // Prescaler /128 for 16MHz -> 125kHz (ADPS2:0 = 111)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    
    // Start first conversion
    ADCSRA |= (1 << ADSC);
}

void ldr_task(void) {
    if (timer_elapsed32(last_ldr_poll) > LDR_POLL_INTERVAL_MS) {
        last_ldr_poll = timer_read32();
        
        // If conversion is complete
        if (!(ADCSRA & (1 << ADSC))) {
            ldr_adc_val = ADC;
            if (ldr_enabled) {
                uint16_t clamped_val = ldr_adc_val;
                // Apply limits
                if (clamped_val < LDR_ADC_DARK) clamped_val = LDR_ADC_DARK;
                if (clamped_val > LDR_ADC_BRIGHT) clamped_val = LDR_ADC_BRIGHT;
                
                // Map to percentage (0 - 100)
                uint8_t brightness_pct = (uint32_t)(clamped_val - LDR_ADC_DARK) * 100 / (LDR_ADC_BRIGHT - LDR_ADC_DARK);
                
                // Add minimum brightness floor
                if (brightness_pct < LDR_MIN_BRIGHTNESS) brightness_pct = LDR_MIN_BRIGHTNESS;
                
                // Calculate absolute val based on max RGB brightness
                uint8_t absolute_val = ((uint16_t)brightness_pct * RGB_MATRIX_MAXIMUM_BRIGHTNESS) / 100;
                
                // Apply hysteresis: only update if delta is significant to avoid flicker
                static uint8_t current_val = 0;
                if (abs((int16_t)absolute_val - (int16_t)current_val) > 2) {
                    current_val = absolute_val;
#ifdef RGB_MATRIX_ENABLE
                    rgb_matrix_sethsv_noeeprom(rgb_matrix_get_hue(), rgb_matrix_get_sat(), current_val);
#endif
                }
            }
            // Start next conversion
            ADCSRA |= (1 << ADSC);
        }
    }
}

void ldr_toggle(void) {
    ldr_enabled = !ldr_enabled;
}
