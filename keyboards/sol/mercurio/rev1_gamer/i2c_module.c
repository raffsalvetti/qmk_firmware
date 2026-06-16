#include "i2c_module.h"
#include "i2c_master.h"
#include "config.h"

void i2c_module_init(void) {
    i2c_init();
}

bool i2c_module_write(uint8_t addr, const uint8_t *data, uint16_t length) {
    return i2c_transmit(addr << 1, data, length, I2C_MODULE_TIMEOUT_MS) == I2C_STATUS_SUCCESS;
}

bool i2c_module_read(uint8_t addr, uint8_t *data, uint16_t length) {
    return i2c_receive(addr << 1, data, length, I2C_MODULE_TIMEOUT_MS) == I2C_STATUS_SUCCESS;
}

bool i2c_module_write_reg(uint8_t addr, uint8_t reg_high, uint8_t reg_low, const uint8_t *data, uint16_t length) {
    uint16_t reg_addr = (reg_high << 8) | reg_low;
    return i2c_write_register16(addr << 1, reg_addr, data, length, I2C_MODULE_TIMEOUT_MS) == I2C_STATUS_SUCCESS;
}

bool i2c_module_read_reg(uint8_t addr, uint8_t reg_high, uint8_t reg_low, uint8_t *data, uint16_t length) {
    uint16_t reg_addr = (reg_high << 8) | reg_low;
    return i2c_read_register16(addr << 1, reg_addr, data, length, I2C_MODULE_TIMEOUT_MS) == I2C_STATUS_SUCCESS;
}
