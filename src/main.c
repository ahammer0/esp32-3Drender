#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/i2c_types.h"
#include "soc/clk_tree_defs.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LCD_I2C_ADDRESS 0x3C
#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH 128

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
      .scl_speed_hz = 100000,
  };

  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
}

void i2c_transmit(uint8_t cmd, uint8_t *data, size_t data_len) {
  uint8_t tries = 0;
  esp_err_t res;
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

  do {
    res = i2c_master_transmit(dev_handle, buffer, buffer_size, -1);
    tries++;
  } while (res != ESP_OK && tries < 5);
  if (tries >= 5) {
    ESP_LOGE("I2C", "Failed to send data");
  }

  free(buffer);
}

///////////////////////////////////////////////////////////////////////////////
/////                                                                   ///////
/////   LCD handling functions                                          ///////
/////                                                                   ///////
///////////////////////////////////////////////////////////////////////////////

typedef struct {
  uint8_t framebuffer[SCREEN_HEIGHT / 8 * SCREEN_WIDTH];
  size_t size;
} LCD;

LCD lcd_init() {
  // i2c_transmit(0xA5, NULL, 0); // force entire display on
  // i2c_transmit(0xA6, NULL, 0); // set display normal

  i2c_transmit(0x8D, NULL, 0); // set charge pump
  i2c_transmit(0x14, NULL, 0);
  //
  i2c_transmit(0x20, NULL, 0); // set hz addr mode
  i2c_transmit(0x00, NULL, 0);

  i2c_transmit(0x21, NULL, 0); // set col addr
  i2c_transmit(0x00, NULL, 0);
  i2c_transmit(127, NULL, 0);

  i2c_transmit(0x22, NULL, 0); // set page addr
  i2c_transmit(0x00, NULL, 0);
  i2c_transmit(7, NULL, 0);

  i2c_transmit(0xAF, NULL, 0); // set display ON

  LCD lcd = {
      .framebuffer = {0},
      .size = SCREEN_WIDTH * SCREEN_HEIGHT / 8,
  };

  return lcd;
}
void lcd_set_pixel(LCD *lcd, int x, int y, bool pixel) {
  if (pixel) {
    lcd->framebuffer[y / 8 * SCREEN_WIDTH + x] |= 1 << (y % 8);
  } else {
    lcd->framebuffer[y / 8 * SCREEN_WIDTH + x] &= ~(1 << (y % 8));
  }
}

void lcd_draw_scr(LCD *lcd) {
  i2c_transmit(0xE3, lcd->framebuffer, (lcd->size));
}

///////////////////////////////////////////////////////////////////////////////
/////                                                                   ///////
/////    MAIN function                                                  ///////
/////                                                                   ///////
///////////////////////////////////////////////////////////////////////////////

void app_main(void) {
  i2c_init();
  LCD lcd = lcd_init();

  printf("sizeof framebuffer %d\n", sizeof(lcd.framebuffer));
  printf("lcd size %d\n", lcd.size);

  while (1) {

    for (int i = 0; i < SCREEN_HEIGHT; i++) {
      for (int j = 0; j < SCREEN_WIDTH; j++) {
        lcd_set_pixel(&lcd, j, i, true);
        lcd_draw_scr(&lcd);
        printf("ici2\n");
      }
    }
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
      for (int j = 0; j < SCREEN_WIDTH; j++) {
        lcd_set_pixel(&lcd, j, i, false);
        lcd_draw_scr(&lcd);
      }
    }
  }

  // printf("I2C BUS device detector\n\n");
  // for (uint16_t i = 0; i <= 127; i++) {
  //   printf("scanning %x ...", i);
  //   esp_err_t res = i2c_master_probe(bus_handle, i, -1);
  //   if (res == ESP_OK) {
  //     printf("found!");
  //   }
  //   printf("\n");
  // }
}
