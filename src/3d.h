#ifndef D3_H
#define D3_H
#import "lcd.h"

typedef struct {
  float x;
  float y;
  float z;
} vec3;
typedef struct {
  vec3 p0;
  vec3 p1;
} vertex_3d;
typedef struct {
  vertex_3d *vertices;
  vec3 center;
  int vertices_count;
} shape_3d;
shape_3d cube(vec3 center, int size);
void rotate(shape_3d *shape, vec3 axis, float angle);
void draw_3d_shape(LCD *lcd, shape_3d *shape);

#endif
