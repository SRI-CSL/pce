/*
 * Wrappers for malloc/realloc/free:
 * abort if out of memory.
 */

#ifndef __MEMALLOC_H
#define __MEMALLOC_H

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "pce_exit_codes.h"

#ifndef __GNUC__
#define __attribute__(x)
#endif

/*
 * Print an error message then call exit(MCSAT_EXIT_OUT_OF_MEMORY)
 */
extern void out_of_memory(void) __attribute__ ((noreturn));

/*
 * Wrappers for malloc/realloc.
 */
extern void *safe_malloc(size_t size) __attribute__ ((malloc)); 
extern void *safe_realloc(void *ptr, size_t size) __attribute__ ((malloc));

/*
 * Safer free: check whether ptr is NULL before calling free.
 *
 * NOTE: C99 specifies that free shall have no effect if ptr
 * is NULL. It's safer to check anyway.
 */
static inline void safe_free(void *ptr) {
  if (ptr != NULL) free(ptr);
}



/*
 * Strings with reference counter
 */
typedef struct {
  uint32_t ref;  // should be plenty; no check for overflow is implemented.
  char str[0];   // real size determined a allocation time.
} string_t;

/*
 * Make a copy of str with ref count 0.
 */
extern char *clone_string(const char *str);


/*
 * header of string s
 */
static inline string_t *string_header(const char *s) {
  return (string_t *) (s - offsetof(string_t, str));
}

/*
 * Increment ref counter for string s
 */
static inline void string_incref(char *s) {
  string_header(s)->ref ++;
}

/*
 * Decrement ref counter for s and free the string if the 
 * counter is zero
 */
static inline void string_decref(char *s) {
  string_t *h;
  h = string_header(s);
  assert(h->ref > 0);
  h->ref --;
  if (h->ref == 0) free(h); 
}


#endif
