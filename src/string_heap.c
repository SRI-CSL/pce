#include <stdio.h>
#include "memalloc.h"
#include "string_heap.h"

void init_string_heap(string_heap_t *s, uint32_t n) {
  if (n >= MAX_STRING_HEAP_SIZE) {
    out_of_memory();
  }
  s->size = n;
  s->index = 0;
  s->data = NULL;
  if (n > 0) {
    s->data = (char *) safe_malloc(n * sizeof(char));
  }
}

void delete_string_heap(string_heap_t *s) {
  safe_free(s->data);
  s->data = NULL;
}

void extend_string_heap(string_heap_t *s) {
  uint32_t n;

  n = s->size + 1;
  n += n >> 1;
  if (n >= MAX_STRING_HEAP_SIZE) {
    out_of_memory();
  }
  s->data = (char *) safe_realloc(s->data, n * sizeof(char));
  s->size = n;
}

void resize_string_heap(string_heap_t *s, uint32_t n) {
  if (n > s->size) {
    if (n >= MAX_STRING_HEAP_SIZE) {
      out_of_memory();
    }
    s->data = (char *) safe_realloc(s->data, n * sizeof(char));
    s->size = n;
  }
}

void string_heap_push(string_heap_t *s, char x) {
  uint32_t i;

  i = s->index;
  if (i >= s->size) {
    extend_string_heap(s);
  }
  s->data[i] = x;
  s->index++;
}

void string_heap_reset(string_heap_t *s) {
  s->index = 0;
}
