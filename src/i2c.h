#ifndef I2C_H
#define I2C_H

#include <stddef.h>
#include <stdint.h>
#define LCD_I2C_ADDRESS 0x3C
void i2c_init(void);
void i2c_transmit(uint8_t cmd, uint8_t *data, size_t data_len);
void i2c_transmit_raw(uint8_t *data, size_t data_len);
#endif
