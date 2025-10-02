#include "i2c.h"
#include "lcd.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
