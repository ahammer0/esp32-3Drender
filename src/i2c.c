#include "i2c.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/i2c_types.h"
#include "soc/clk_tree_defs.h"

i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t dev_handle;
///////////////////////////////////////////////////////////////////////////////
/////                                                                   ///////
/////   I2C handling functions                                          ///////
/////                                                                   ///////
///////////////////////////////////////////////////////////////////////////////
void i2c_init() {
  i2c_master_bus_config_t i2c_mst_config = {
      .clk_source = LP_I2C_SCLK_DEFAULT,
      .i2c_port = LP_I2C_NUM_0,
      .sda_io_num = GPIO_NUM_6,
      .scl_io_num = GPIO_NUM_7,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = LCD_I2C_ADDRESS,
      .scl_speed_hz = 400000,
  };

  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
}

void i2c_transmit_raw(uint8_t *data, size_t data_len) {
  uint8_t tries = 0;
  esp_err_t res;
  do {
    res = i2c_master_transmit(dev_handle, data, data_len, -1);
    tries++;
  } while (res != ESP_OK && tries < 5);
  if (tries >= 5) {
    ESP_LOGE("I2C", "Failed to send data");
  }
}

void i2c_transmit(uint8_t cmd, uint8_t *data, size_t data_len) {
  size_t buffer_size = data_len == 0 ? 2 : data_len + 3;
  uint8_t *buffer = malloc(sizeof(uint8_t) * buffer_size);

  // false if following bytes is data only
  bool co = true;

  // next byte is command or data
  // false following byte is command
  // true following byte will be stored in gddram ( column address pointer
  // increased automaticly )
  bool d_c = false;

  uint8_t ctrl = 0x00 | (co << 7) | (d_c << 6);
  buffer[0] = ctrl;
  buffer[1] = cmd;

  if (data_len > 0) {
    co = false;
    d_c = true;
    ctrl = 0x00 | (co << 7) | (d_c << 6);
    buffer[2] = ctrl;
    for (size_t i = 0; i < data_len; i++) {
      buffer[i + 3] = data[i];
    }
  }

  i2c_transmit_raw(buffer, buffer_size);

  free(buffer);
}
