#pragma once
#include <stdint.h>

void piezo_effect_keyclick(void);
void piezo_effect_layer_change(uint8_t new_layer);
void piezo_effect_startup(void);
void piezo_effects_task(void);
