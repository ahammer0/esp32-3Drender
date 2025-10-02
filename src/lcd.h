#ifndef LCD_H
#define LCD_H

#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH 128
#import "h_set.h"
#import <stdlib.h>

typedef struct {
  uint8_t framebuffer[SCREEN_HEIGHT / 8][SCREEN_WIDTH];
  size_t size;
  h_set_t *dirty_zones;
} LCD;
LCD lcd_init();
void lcd_set_pixel(LCD *lcd, int x, int y, bool pixel);
void lcd_draw_scr_diff(LCD *lcd);
void lcd_clr_scr(LCD *lcd);
void lcd_draw_line(LCD *lcd, int x0, int y0, int x1, int y1, bool pixel);
#endif
