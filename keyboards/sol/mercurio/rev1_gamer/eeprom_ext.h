#pragma once
#include <stdint.h>
#include <stdbool.h>

void eeprom_ext_init(void);
bool eeprom_ext_read(uint32_t mem_addr, uint8_t *data, uint16_t length);
bool eeprom_ext_write_page(uint32_t mem_addr, const uint8_t *data, uint16_t length);
