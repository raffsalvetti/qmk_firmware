#pragma once
#include <stdint.h>

void piezo_init(void);
void piezo_task(void);
void piezo_play(uint8_t ocr2_val, uint16_t duration_ms);
void piezo_stop(void);
