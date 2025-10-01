#ifndef H_SET_H
#define H_SET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct h_set_t h_set_t;

typedef struct node {
  uint8_t page;
  uint8_t segment;
  struct node *next;
} h_set_node_t;

struct h_set_t {
  uint8_t max_page;
  uint8_t max_segment;
  h_set_node_t **buckets;
  h_set_node_t *head;
  int size;
};
typedef struct {
  const h_set_t *set;
  h_set_node_t *current;
} h_set_iter_t;

h_set_t *h_set_new(uint8_t page_max, uint8_t segment_max);
void h_set_free(h_set_t *set);
void h_set_add(h_set_t *set, uint8_t page, uint8_t segment);
bool h_set_has(h_set_t *set, uint8_t page, uint8_t segment);
void h_set_clear(h_set_t *set);

h_set_iter_t h_set_iter(h_set_t *set);
void h_set_next(h_set_iter_t *it);

#endif
