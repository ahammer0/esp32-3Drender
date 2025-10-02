#include "lcd.h"
#include "h_set.h"
#include "i2c.h"
#include <stdlib.h>
///////////////////////////////////////////////////////////////////////////////
/////                                                                   ///////
/////   LCD handling functions                                          ///////
/////                                                                   ///////
///////////////////////////////////////////////////////////////////////////////

LCD lcd_init() {

  i2c_transmit(0x8D, NULL, 0); // set charge pump
  i2c_transmit(0x14, NULL, 0);
  //
  i2c_transmit(0x20, NULL, 0); // set page addr mode
  i2c_transmit(0x02, NULL, 0);

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
