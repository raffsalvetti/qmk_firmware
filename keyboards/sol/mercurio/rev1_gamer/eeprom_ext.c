#include "eeprom_ext.h"
#include "i2c_module.h"
#include "config.h"
#include "wait.h"

void eeprom_ext_init(void) {
    // Rely on i2c_module_init which is called separately.
}

static uint8_t get_device_addr(uint32_t mem_addr) {
    // 24LC1025 has two blocks. The block select bit is bit 16 of the address.
    // Address 0x00000 - 0x0FFFF uses EEPROM_EXT_ADDR (0x50)
    // Address 0x10000 - 0x1FFFF uses EEPROM_EXT_ADDR_B1 (0x54)
    if (mem_addr & 0x10000) {
        return EEPROM_EXT_ADDR_B1;
    }
    return EEPROM_EXT_ADDR;
}

bool eeprom_ext_read(uint32_t mem_addr, uint8_t *data, uint16_t length) {
    uint8_t dev_addr = get_device_addr(mem_addr);
    uint8_t reg_high = (mem_addr >> 8) & 0xFF;
    uint8_t reg_low = mem_addr & 0xFF;
    
    return i2c_module_read_reg(dev_addr, reg_high, reg_low, data, length);
}

bool eeprom_ext_write_page(uint32_t mem_addr, const uint8_t *data, uint16_t length) {
    // Max page write length for 24LC1025 is 128 bytes, and must not cross page boundaries.
    if (length > EEPROM_EXT_PAGE_SIZE) {
        length = EEPROM_EXT_PAGE_SIZE;
    }
    
    uint8_t dev_addr = get_device_addr(mem_addr);
    uint8_t reg_high = (mem_addr >> 8) & 0xFF;
    uint8_t reg_low = mem_addr & 0xFF;
    
    bool success = i2c_module_write_reg(dev_addr, reg_high, reg_low, data, length);
    if (success) {
        // The 24LC1025 requires a 5ms internal write cycle.
        // We must wait before any subsequent read/write operations.
        wait_ms(EEPROM_EXT_WRITE_MS);
    }
    return success;
}
