/**************************************
 *  WRAPPERS FOR MALLOC/REALLOC/FREE  *
 *************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "memalloc.h"
#include "pce_exit_codes.h"

/*
 * Fatal error: out of memory
 */
void out_of_memory() {
  fprintf(stderr, "Out of memory\n");
  exit(PCE_EXIT_OUT_OF_MEMORY);
}


/*
 * Local malloc: abort if out of memory.
 *
 * Special case: if size = 0, malloc(size) may
 * return NULL on some systems, but that does not
 * mean we're out of memory.
 */
void *safe_malloc(size_t size) {
  void *tmp;

  tmp = malloc(size);
  if (tmp == NULL && size > 0) {
    out_of_memory();
  }

  return tmp;
}

/*
 * Safer realloc to support lazy allocation.
 * If ptr == NULL, call malloc otherwise call realloc.
 * Abort if out of memory.
 *
 * NOTE: C99 specifies that realloc should behave like
 * malloc if ptr is NULL. This is what the Linux default
 * malloc does, but it's not clear whether other malloc 
 * implementations (e.g., on MacOSX) follow the standard.
 * It's safer to check whether ptr is NULL and 
 * call malloc or realloc accordingly.
 *
 * size must be positive: realloc(p, 0) is the same as free(ptr).
 */
void *safe_realloc(void *ptr, size_t size) {
  void *tmp;

  assert(size > 0);
  if (ptr == NULL) {
    tmp = malloc(size);
  } else {
    tmp = realloc(ptr, size);
  }
  if (tmp == NULL) out_of_memory();

  return tmp;
}


/*
 * Allocate character string with reference counting
 * Make a copy of str and set ref count to 0.
 */
char *clone_string(const char *str) {
  string_t *tmp;
  tmp = (string_t *) safe_malloc(sizeof(string_t) + strlen(str) + 1);
  tmp->ref = 0;
  strcpy(tmp->str, str);
  return tmp->str;
}
