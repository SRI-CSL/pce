/*
 * Generic vectors (resizable arrays)
 * Used in egraph and sat_solvers
 */


#include "memalloc.h"
#include "vectors.h"
#include "int_array_sort.h"


/*
 * Integer vectors
 */
void init_ivector(ivector_t *v, uint32_t n) {
  if (n >= MAX_VECTOR_SIZE) {
    out_of_memory();
  }
  v->capacity = n;
  v->size = 0;
  v->data = NULL;
  if (n > 0) {
    v->data = (int32_t *) safe_malloc(n * sizeof(int32_t));
  }
}

void delete_ivector(ivector_t *v) {
  safe_free(v->data);
  v->data = NULL;
}

void  extend_ivector(ivector_t *v) {
  uint32_t n;

  n = v->capacity + 1;
  n += n >> 1;
  if (n >= MAX_VECTOR_SIZE) {
    out_of_memory();
  }
  v->data = (int32_t *) safe_realloc(v->data, n * sizeof(int32_t));
  v->capacity = n;
}

void resize_ivector(ivector_t *v, uint32_t n) {
  if (n > v->capacity) {
    if (n >= MAX_VECTOR_SIZE) {
      out_of_memory();
    }
    v->data = (int32_t *) safe_realloc(v->data, n * sizeof(int32_t));
    v->capacity = n;
  }
}


/*
 * Sort and remove duplicates
 */
void ivector_remove_duplicates(ivector_t *v) {
  uint32_t n, i, j;
  int32_t x, y, *a;

  n = v->size;
  if (n >= 2) {
    a = v->data;
    int_array_sort(a, n);

    x = a[0];
    j = 1;
    for (i=1; i<n; i++) {
      y = a[i];
      if (x != y) {
	a[j] = y;
	x = y;
	j ++;	
      }
    }
    v->size = j;
  }
}



/*
 * Pointer vectors
 */
void init_pvector(pvector_t *v, uint32_t n) {
  if (n >= MAX_VECTOR_SIZE) {
    out_of_memory();
  }
  v->capacity = n;
  v->size = 0;
  v->data = NULL;
  if (n > 0) {
    v->data = (void **) safe_malloc(n * sizeof(void *));
  }
}

void delete_pvector(pvector_t *v) {
  safe_free(v->data);
  v->data = NULL;
}

void extend_pvector(pvector_t *v) {
  uint32_t n;

  n = v->capacity + 1;
  n += n >> 1;
  if (n >= MAX_VECTOR_SIZE) {
    out_of_memory();
  }
  v->data = (void **) safe_realloc(v->data, n * sizeof(void *));
  v->capacity = n;
}

void resize_pvector(pvector_t *v, uint32_t n) {
  if (n > v->capacity) {
    if (n >= MAX_VECTOR_SIZE) {
      out_of_memory();
    }
    v->data = (void **) safe_realloc(v->data, n * sizeof(void *));
    v->capacity = n;
  }
}

