/*
 * Generic vectors (resizable arrays)
 * Used in egraph and sat_solvers
 */

#ifndef __VECTORS_H
#define __VECTORS_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/*
 * Vector of signed 32bit integers
 */
typedef struct ivector_s {
  uint32_t capacity;
  uint32_t size;
  int32_t *data;
} ivector_t;


/*
 * Vector of pointers
 */
typedef struct pvector_s {
  uint32_t capacity;
  uint32_t size;
  void **data;
} pvector_t;


/*
 * Vector of doubles
 */
typedef struct dvector_s {
  uint32_t capacity;
  uint32_t size;
  double *data;
} dvector_t;


/*
 * Default initial size and max size
 */
#define DEFAULT_VECTOR_SIZE 10
#define MAX_VECTOR_SIZE (UINT32_MAX/8)


/*
 * Operations:
 * init_vector(v, n) with  n = size
 * delete_vector(v)
 * extend(v): increase size by 50%
 * resize(v, n): make v large enough for at least n elements
 * push(v, x): add x at the end of v
 * reset(v): empty x
 * shrink(v, n): remove all elements but n
 */

/*
 * Integer vectors
 */
extern void init_ivector(ivector_t *v, uint32_t n);

extern void delete_ivector(ivector_t *v);

extern void extend_ivector(ivector_t *v);

extern void resize_ivector(ivector_t *v, uint32_t n);

static inline void ivector_push(ivector_t *v, int32_t x) {
  uint32_t i;

  i = v->size;
  if (i >= v->capacity) {
    extend_ivector(v);
  }
  v->data[i] = x;
  v->size = i+1;
}

static inline void ivector_reset(ivector_t *v) {
  v->size = 0;
}

static inline void ivector_shrink(ivector_t *v, uint32_t n) {
  assert(n <= v->size);
  v->size = n;
}


/*
 * Remove duplicates in an integer vector
 */
extern void ivector_remove_duplicates(ivector_t *v);




/*
 * Pointer vectors
 */
extern void init_pvector(pvector_t *v, uint32_t n);

extern void delete_pvector(pvector_t *v);

extern void extend_pvector(pvector_t *v);

extern void resize_pvector(pvector_t *v, uint32_t n);

static void pvector_push(pvector_t *v, void *p) {
  uint32_t i;

  i = v->size;
  if (i >= v->capacity) {
    extend_pvector(v);
  }
  v->data[i] = p;
  v->size = i+1;
}

static inline void pvector_reset(pvector_t *v) {
  v->size = 0;
}

static inline void pvector_shrink(pvector_t *v, uint32_t n) {
  assert(n <= v->size);
  v->size = n;
}

/*
 * Double vectors
 */

extern void init_dvector(dvector_t *v, uint32_t n);

extern void delete_dvector(dvector_t *v);

extern void extend_dvector(dvector_t *v);

extern void resize_dvector(dvector_t *v, uint32_t n);

static inline void dvector_push(dvector_t *v, double x) {
  uint32_t i;

  i = v->size;
  if (i >= v->capacity) {
    extend_dvector(v);
  }
  v->data[i] = x;
  v->size = i+1;
}

static inline void dvector_reset(dvector_t *v) {
  v->size = 0;
}

static inline void dvector_shrink(dvector_t *v, uint32_t n) {
  assert(n <= v->size);
  v->size = n;
}

#endif /* __VECTORS_H */
