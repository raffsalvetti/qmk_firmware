#include "piezo_effects.h"
#include "piezo.h"
#include "config.h"
#include "quantum.h"

// State machine for multi-note effects (startup, layer change)
static uint8_t effect_state = 0;
static uint8_t effect_type = 0; // 1 = startup, 2 = layer up, 3 = layer down
static uint32_t effect_timer = 0;

void piezo_effect_keyclick(void) {
    // Only play if no other effect is active
    if (effect_state == 0) {
        piezo_play(PIEZO_NOTE_G5, PIEZO_CLICK_DURATION_MS);
    }
}

void piezo_effect_layer_change(uint8_t new_layer) {
    static uint8_t old_layer = 0;
    if (new_layer == old_layer) return;
    
    effect_type = (new_layer > old_layer) ? 1 : 2;
    if (new_layer == 0) effect_type = 3;
    
    old_layer = new_layer;
    effect_state = 1;
    effect_timer = timer_read32();
    piezo_play(effect_type == 2 ? PIEZO_NOTE_E5 : PIEZO_NOTE_G5, PIEZO_LAYER_TONE_MS);
}

void piezo_effect_startup(void) {
    effect_type = 1;
    effect_state = 1;
    effect_timer = timer_read32();
    piezo_play(PIEZO_NOTE_C5, 150);
}

void piezo_effects_task(void) {
    if (effect_state == 1) {
        uint16_t delay = (effect_type == 1) ? 150 : PIEZO_LAYER_TONE_MS;
        if (timer_elapsed32(effect_timer) > delay) {
            effect_state = 2;
            effect_timer = timer_read32();
            if (effect_type == 1) {
                piezo_play(PIEZO_NOTE_E5, 150);
            } else if (effect_type == 2) {
                piezo_play(PIEZO_NOTE_G5, PIEZO_LAYER_TONE_MS);
            } else if (effect_type == 3) {
                piezo_play(PIEZO_NOTE_D5, PIEZO_LAYER_TONE_MS);
            }
        }
    } else if (effect_state == 2) {
        uint16_t delay = (effect_type == 1) ? 150 : PIEZO_LAYER_TONE_MS;
        if (timer_elapsed32(effect_timer) > delay) {
            if (effect_type == 1) {
                effect_state = 3;
                effect_timer = timer_read32();
                piezo_play(PIEZO_NOTE_G5, 150);
            } else {
                effect_state = 0;
            }
        }
    } else if (effect_state == 3) {
        if (effect_type == 1) {
            if (timer_elapsed32(effect_timer) > 150) {
                effect_state = 4;
                effect_timer = timer_read32();
                piezo_play(PIEZO_NOTE_B5, 300);
            }
        } else {
            if (timer_elapsed32(effect_timer) > (PIEZO_LAYER_TONE_MS * 2)) {
                effect_state = 0;
            }
        }
    } else if (effect_state == 4) {
        if (timer_elapsed32(effect_timer) > 300) {
            effect_state = 0;
        }
    }
}
