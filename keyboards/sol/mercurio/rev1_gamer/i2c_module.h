#pragma once
#include <stdint.h>
#include <stdbool.h>

void i2c_module_init(void);
bool i2c_module_write(uint8_t addr, const uint8_t *data, uint16_t length);
bool i2c_module_read(uint8_t addr, uint8_t *data, uint16_t length);
bool i2c_module_write_reg(uint8_t addr, uint8_t reg_high, uint8_t reg_low, const uint8_t *data, uint16_t length);
bool i2c_module_read_reg(uint8_t addr, uint8_t reg_high, uint8_t reg_low, uint8_t *data, uint16_t length);
