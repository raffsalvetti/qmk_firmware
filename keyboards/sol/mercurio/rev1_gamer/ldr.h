#pragma once

#include <stdbool.h>
#include <stdint.h>

void ldr_init(void);
void ldr_task(void);
void ldr_toggle(void);

extern bool ldr_enabled;
extern uint16_t ldr_adc_val;
