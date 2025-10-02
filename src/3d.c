#include "3d.h"
#include <math.h>
#include <stdlib.h>

typedef struct {
  float x;
  float y;
} vec2;
typedef struct {
  float d[3][3];
} mat3;


typedef struct {
  vec2 p0;
  vec2 p1;
} vertex_2d;

typedef struct {
  vertex_2d *vertices;
  int vertices_count;
} shape_2d;

vec2 mult2(vec2 a, vec2 b) {
  return (vec2){
      .x = a.x * b.x,
      .y = a.y * b.y,
  };
}
vec3 mult3(vec3 a, vec3 b) {
  return (vec3){
      .x = a.x * b.x,
      .y = a.y * b.y,
      .z = a.z * b.z,
  };
}
vec2 add2(vec2 a, vec2 b) {
  return (vec2){
      .x = a.x + b.x,
      .y = a.y + b.y,
  };
}
vec3 vec3_add(vec3 a, vec3 b) {
  return (vec3){
      .x = a.x + b.x,
      .y = a.y + b.y,
      .z = a.z + b.z,
  };
}
vec3 vec3_sub(vec3 a, vec3 b){
  return (vec3){
      .x = a.x - b.x,
      .y = a.y - b.y,
      .z = a.z - b.z,
  };

}

float vec3_mult(vec3 a, vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 vec_3_to_normalized(vec3 v) {
  float n = 1.0f / sqrtf(vec3_mult(v, v));
  return mult3(v, (vec3){n, n, n});
}

mat3 mat3_add_mat3(mat3 a, mat3 b) {
  mat3 res={0};
  for(int i=0;i<3;i++){
    for(int j=0;j<3;j++){
        res.d[i][j]+=a.d[i][j]+b.d[i][j];
    }
  }
  return res;
}
mat3 scal_mult_mat3(float s, mat3 m) {
  mat3 res={0};
  for(int i=0;i<3;i++){
    for(int j=0;j<3;j++){
      res.d[i][j]=s*m.d[i][j];
    }
  }
  return res;

}
mat3 mat3_mult_mat3(mat3 a, mat3 b) {
  mat3 res={0};
  for(int i=0;i<3;i++){
    for(int j=0;j<3;j++){
      for(int k=0;k<3;k++){
        res.d[i][j]+=a.d[i][k]*b.d[k][j];
      }
    }
  }
  return res;
}
vec3 vec3_mult_mat3(vec3 v,mat3 m){
  vec3 res={0};
  res.x = m.d[0][0]*v.x + m.d[0][1]*v.y + m.d[0][2]*v.z;
  res.y = m.d[1][0]*v.x + m.d[1][1]*v.y + m.d[1][2]*v.z;
  res.z = m.d[2][0]*v.x + m.d[2][1]*v.y + m.d[2][2]*v.z;
  return res;
}

void vec3_rotate(vec3 *v, vec3 axis, float angle) {
  vec3 n = vec_3_to_normalized(axis);
  mat3 I = {
      .d =
          {
              {1, 0, 0},
              {0, 1, 0},
              {0, 0, 1},
          },
  };
  mat3 Q = {
      .d =
          {
              {0, -1 * n.z, n.y},
              {n.z, 0, -1 * n.x},
              {-1 * n.y, n.x, 0},
          },
  };
  mat3 R = mat3_add_mat3(
      I, 
      mat3_add_mat3(
        scal_mult_mat3(
          sin(angle), 
          Q),
        scal_mult_mat3(
          (1 - cos(angle)), 
          mat3_mult_mat3(Q,Q)
        )
      )
  );
  *v = vec3_mult_mat3(*v,R);
}

shape_3d cube(vec3 center, int size) {
  const vec3 p[] = {
      {-1, 1, 1},  {-1, 1, -1},  {1, 1, -1},  {1, 1, 1},
      {-1, -1, 1}, {-1, -1, -1}, {1, -1, -1}, {1, -1, 1},
  };

  vertex_3d *v = malloc(sizeof(vertex_3d) * 12);
  // upper face
  v[0] = (vertex_3d){p[0], p[1]};
  v[1] = (vertex_3d){p[1], p[2]};
  v[2] = (vertex_3d){p[2], p[3]};
  v[3] = (vertex_3d){p[3], p[0]};
  // lower face
  v[4] = (vertex_3d){p[4], p[5]};
  v[5] = (vertex_3d){p[5], p[6]};
  v[6] = (vertex_3d){p[6], p[7]};
  v[7] = (vertex_3d){p[7], p[8]};
  // verticals
  v[8] = (vertex_3d){p[4], p[0]};
  v[9] = (vertex_3d){p[5], p[1]};
  v[10] = (vertex_3d){p[6], p[2]};
  v[11] = (vertex_3d){p[7], p[3]};

  // scale it
  for (int i = 0; i < 12; i++) {
    v[i] = (vertex_3d){
        .p0 = vec3_add(mult3(v[i].p0,
                    (vec3){(float)size / 2, (float)size / 2, (float)size / 2}),center),
        .p1 = vec3_add(mult3(v[i].p1,
                    (vec3){(float)size / 2, (float)size / 2, (float)size / 2}),center)};
  }


  return (shape_3d){
      .vertices_count = 12,
      .vertices = v,
      .center = center,
  };
}

void rotate(shape_3d *shape, vec3 axis, float angle) {
  for(int i=0;i<shape->vertices_count;i++){
    shape->vertices[i].p0=vec3_sub(shape->vertices[i].p0,shape->center);
    shape->vertices[i].p1=vec3_sub(shape->vertices[i].p1,shape->center);
    vec3_rotate(&shape->vertices[i].p0,axis,angle);
    vec3_rotate(&shape->vertices[i].p1,axis,angle);
    shape->vertices[i].p0=vec3_add(shape->vertices[i].p0,shape->center);
    shape->vertices[i].p1=vec3_add(shape->vertices[i].p1,shape->center);
  }
}

shape_2d project(shape_3d *shape) {
  vertex_2d *projected_vertices =
      malloc(sizeof(vertex_2d) * shape->vertices_count);
  for (int i = 0; i < shape->vertices_count; i++) {
    const vertex_3d v3 = shape->vertices[i];
    const vertex_2d v2 = {
        .p0.x = v3.p0.x,
        .p0.y = v3.p0.y,
        .p1.x = v3.p1.x,
        .p1.y = v3.p1.y,
    };
    projected_vertices[i] = v2;
  }

  shape_2d s = {
      .vertices_count = shape->vertices_count,
      .vertices = projected_vertices,
  };
  return s;
}

void draw_2d_vertice(LCD *lcd, vertex_2d v) {
  lcd_draw_line(lcd, v.p0.x, v.p0.y, v.p1.x, v.p1.y, true);
}

void draw_3d_shape(LCD *lcd, shape_3d *shape) {
  shape_2d sh = project(shape);

  for (int i = 0; i < sh.vertices_count; i++) {
    draw_2d_vertice(lcd, sh.vertices[i]);
  }
  free(sh.vertices);
}
