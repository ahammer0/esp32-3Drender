#include "h_set.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

h_set_t *h_set_new(uint8_t page_max, uint8_t segment_max) {

  h_set_t *set = malloc(sizeof(h_set_t));
  set->max_page = page_max;
  set->max_segment = segment_max;
  set->head = NULL;
  set->buckets =
      calloc(1, sizeof(h_set_node_t *) * set->max_page * set->max_segment);
  set->size = 0;
  return set;
}

int hash(h_set_t *set, uint8_t page, uint8_t segment) {
  return page * set->max_segment + segment;
}

bool h_set_has(h_set_t *set, uint8_t page, uint8_t segment) {
  return set->buckets[hash(set, page, segment)] != NULL;
}

void h_set_add(h_set_t *set, uint8_t page, uint8_t segment) {
  if (h_set_has(set, page, segment))
    return;

  h_set_node_t *newNode = malloc(sizeof(h_set_node_t));
  newNode->page = page;
  newNode->segment = segment;
  newNode->next = set->head;
  set->head = newNode;
  set->size++;

  int index = hash(set, page, segment);
  set->buckets[index] = newNode;
}
void h_set_clear(h_set_t *set) {
  for (h_set_iter_t it = h_set_iter(set); it.current != NULL;) {
    // printf("dans boucle hash:%d\n",
    //        hash(set, it.current->page, it.current->segment));
    h_set_node_t *node =
        set->buckets[hash(set, it.current->page, it.current->segment)];
    set->buckets[hash(set, it.current->page, it.current->segment)] = NULL;
    h_set_next(&it);
    free(node);
  }
  set->size = 0;
  set->head = NULL;
}

// Iterator functions
h_set_iter_t h_set_iter(h_set_t *set) {
  h_set_iter_t iter = {
      .current = set->head,
      .set = set,
  };
  return iter;
}

void h_set_next(h_set_iter_t *it) { it->current = it->current->next; }

// int main() {
//   printf("start shit\n");
//
//   h_set_t *s = h_set_new(8, 128);
//   printf("set created\n");
//   printf("has page 2, segment 3: %b\n", h_set_has(s, 2, 3));
//
//   printf("addding page 2, segment 3\n");
//   h_set_add(s, 2, 3);
//   printf("has page 2, segment 3: %b\n", h_set_has(s, 2, 3));
//
//   printf("addding page 2, segment 4\n");
//   h_set_add(s, 2, 4);
//   printf("addding page 3, segment 4\n");
//   h_set_add(s, 3, 4);
//
//   printf("displaying all items using iterator\n");
//   h_set_iter_t it = h_set_iter(s);
//
//   while (it.current != NULL) {
//     printf("page %d,segment %d\n", it.current->page, it.current->segment);
//     h_set_next(&it);
//   }
// }
