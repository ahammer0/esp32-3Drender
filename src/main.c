#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "h_set.h"
#include "hal/i2c_types.h"
#include "soc/clk_tree_defs.h"
#include <math.h>
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

///////////////////////////////////////////////////////////////////////////////
/////                                                                   ///////
/////   LCD handling functions                                          ///////
/////                                                                   ///////
///////////////////////////////////////////////////////////////////////////////

typedef struct {
  uint8_t framebuffer[SCREEN_HEIGHT / 8][SCREEN_WIDTH];
  size_t size;
  h_set_t *dirty_zones;
} LCD;

LCD lcd_init() {
  // i2c_transmit(0xA5, NULL, 0); // force entire display on
  // i2c_transmit(0xA6, NULL, 0); // set display normal

  i2c_transmit(0x8D, NULL, 0); // set charge pump
  i2c_transmit(0x14, NULL, 0);
  //
  i2c_transmit(0x20, NULL, 0); // set page addr mode
  i2c_transmit(0x02, NULL, 0);

  // i2c_transmit(0x21, NULL, 0); // set col addr
  // i2c_transmit(0x00, NULL, 0);
  // i2c_transmit(127, NULL, 0);
  //
  // i2c_transmit(0x22, NULL, 0); // set page addr
  // i2c_transmit(0x00, NULL, 0);
  // i2c_transmit(7, NULL, 0);

  i2c_transmit(0xAF, NULL, 0); // set display ON

  LCD lcd = {
      .framebuffer = {{0}},
      .size = SCREEN_WIDTH * SCREEN_HEIGHT / 8,
      .dirty_zones = h_set_new(8, 128),
  };
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 128; j++) {
      h_set_add(lcd.dirty_zones, i, j);
    }
  }

  return lcd;
}
void lcd_set_pixel(LCD *lcd, int x, int y, bool pixel) {
  // TODO: register dirty columns
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
    return;

  int seg = x;
  int page = y / 8;
  h_set_add(lcd->dirty_zones, page, seg);
  if (pixel) {
    lcd->framebuffer[page][seg] |= 1 << (y % 8);
  } else {
    lcd->framebuffer[page][seg] &= ~(1 << (y % 8));
  }
}

void lcd_draw_scr_diff(LCD *lcd) {
  if (lcd->dirty_zones->size == 0)
    return;
  const uint8_t frame_size = 8;
  const size_t buffer_size = frame_size * lcd->dirty_zones->size;
  uint8_t *buffer = malloc(sizeof(uint8_t) * buffer_size);
  int i = 0;

  for (h_set_iter_t it = h_set_iter(lcd->dirty_zones); it.current != NULL;
       h_set_next(&it)) {
    // control byte :
    // 0x80  -> command mode
    // OxC0  --> data mode

    // set page address
    buffer[i * frame_size + 0] = 0x80;                    // command mode
    buffer[i * frame_size + 1] = 0xB0 | it.current->page; // set page

    // set column address
    buffer[i * frame_size + 2] = 0x80; // next is command
    // set column lower
    buffer[i * frame_size + 3] = 0x00 | (it.current->segment & 0x0F);

    buffer[i * frame_size + 4] = 0x80; // next is command
    buffer[i * frame_size + 5] =
        0x10 | (it.current->segment >> 4); // set column upper

    // sending data
    buffer[i * frame_size + 6] = 0xC0; // next is data
    buffer[i * frame_size + 7] =
        lcd->framebuffer[it.current->page][it.current->segment];
    i++;
  }

  i2c_transmit_raw(buffer, buffer_size);
  h_set_clear(lcd->dirty_zones);
  free(buffer);
}

void lcd_clr_scr(LCD *lcd) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 128; j++) {
      if (lcd->framebuffer[i][j] != 0x00) {
        h_set_add(lcd->dirty_zones, i, j);
        lcd->framebuffer[i][j] = 0x00;
      }
    }
  }
}
void lcd_draw_line(LCD *lcd, int x0, int y0, int x1, int y1, bool pixel) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);

  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;

  int err = dx - dy;

  while (true) {
    lcd_set_pixel(lcd, x0, y0, pixel); // Trace le point courant

    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;

    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }

    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
/////                                                                   ///////
/////    MAIN function                                                  ///////
/////                                                                   ///////
///////////////////////////////////////////////////////////////////////////////

void app_main(void) {
  i2c_init();
  LCD lcd = lcd_init();

  while (1) {
    lcd_clr_scr(&lcd);
    lcd_draw_scr_diff(&lcd);

    // BOOM CIRCLE
    float div_ang = 5.0;
    float div_rad = 20.0;

    int prev_x, prev_y, next_x, next_y;
    prev_x = SCREEN_WIDTH / 2;
    prev_y = SCREEN_HEIGHT / 2;
    for (size_t i = 0; i < 1000; i++) {

      next_x = SCREEN_WIDTH / 2 + i / div_rad * cos(M_2_PI * i / div_ang);
      next_y = SCREEN_HEIGHT / 2 + i / div_rad * sin(M_2_PI * i / div_ang);
      lcd_draw_line(&lcd, prev_x, prev_y, next_x, next_y, true);
      prev_x = next_x;
      prev_y = next_y;
      if (i % (int)div_ang == 0) {
        lcd_draw_scr_diff(&lcd);
      }
    }
    for (size_t i = 0; i < 1000; i++) {
      next_x = SCREEN_WIDTH / 2 + i / div_rad * cos(M_2_PI * i / div_ang);
      next_y = SCREEN_HEIGHT / 2 + i / div_rad * sin(M_2_PI * i / div_ang);
      lcd_draw_line(&lcd, prev_x, prev_y, next_x, next_y, false);
      prev_x = next_x;
      prev_y = next_y;

      if (i % (int)div_ang == 0) {
        lcd_draw_scr_diff(&lcd);
      }
    }

    // FULL SCREEN SCAN ANIM
    // for (int i = 0; i < SCREEN_HEIGHT; i += 2) {
    //   for (int j = 0; j < SCREEN_WIDTH; j += 2) {
    //     lcd_set_pixel(&lcd, j, i, true);
    //     lcd_set_pixel(&lcd, j + 1, i + 1, true);
    //     lcd_draw_scr_diff(&lcd);
    //   }
    // }
    // for (int i = 0; i < SCREEN_HEIGHT; i++) {
    //   for (int j = 0; j < SCREEN_WIDTH; j++) {
    //     lcd_set_pixel(&lcd, j, i, false);
    //     lcd_draw_scr_diff(&lcd);
    //   }
    // }
  }

  // printf("I2C BUS device detector\n\n");
  // for (uint16_t i = 0; i <= 127; i++) {
  //   printf("scanning %x ...", i);
  //   esp_err_t res = i2c_master_probe(bus_handle, i, -1);
  //   if (res == ESP_OK) {
  //     printf("found!");
  //   }
  //   printf("\n");
}
