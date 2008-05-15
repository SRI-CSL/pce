
#ifndef __STRING_HEAP_H
#define __STRING_HEAP_H

/* This implements an array for keeping strings.  Each string kept here is
 * \0 terminated, and the index is used to access the string
 */

typedef struct string_heap_s {
  uint32_t size;
  uint32_t index;
  char *data;
} string_heap_t;

#define DEFAULT_STRING_HEAP_SIZE 20000
#define MAX_STRING_HEAP_SIZE UINT32_MAX

/*
 * Operations:
 * init_string_heap(s, n) with  n = size
 * delete_string_heap(s)
 * extend_string_heap(s): increase size by 50%
 * resize_string_heap(s, n): make s large enough for at least n elements
 * string_heap_push(s, x): add x at the end of s
 * string_heap_reset(s): empty s
 */

extern void init_string_heap(string_heap_t *s, uint32_t n);

extern void delete_string_heap(string_heap_t *s);

extern void extend_string_heap(string_heap_t *s);

extern void resize_string_heap(string_heap_t *s, uint32_t n);

extern void string_heap_push(string_heap_t *s, char x);

extern void string_heap_reset(string_heap_t *s);

#endif /* __STRING_HEAP_H */

