/*
 * Maps 32bit integer arrays to 32bit integers (all signed)
 * Assumes that keys are non-negative (NS: Check this)
 */

#include <assert.h>
#include <stdbool.h>
#include "memalloc.h"
#include "array_hash_map.h"
#include "hash_functions.h"

/*
 * Initialization:
 * - n = initial size, must be a power of 2
 * - if n = 0, the default size is used
 */
void init_array_hmap(array_hmap_t *hmap, uint32_t n) {
  uint32_t i;
  array_hmap_pair_t *tmp;
  //check that n is a power of 2
#ifndef NDEBUG
  uint32_t n2;
  n2 = n;
  while (n2 > 1) {
    assert((n2 & 1) == 0);
    n2 >>= 1;
  }
#endif

  if (n == 0) {
    n = ARRAY_HMAP_DEFAULT_SIZE;
  }

  if (n >= ARRAY_HMAP_MAX_SIZE) {
    out_of_memory();
  }

  tmp = (array_hmap_pair_t *) safe_malloc(n * sizeof(array_hmap_pair_t));
  for (i=0; i<n; i++) {
    tmp[i].key = ARRAY_HASH_EMPTY_KEY;
  }

  hmap->data = tmp;
  hmap->size = n;
  hmap->nelems = 0;
  hmap->ndeleted = 0;

  hmap->resize_threshold = (uint32_t)(n * ARRAY_HMAP_RESIZE_RATIO);
  hmap->cleanup_threshold = (uint32_t) (n * ARRAY_HMAP_CLEANUP_RATIO);
}


/*
 * Free memory
 */
void delete_array_hmap(array_hmap_t *hmap) {
  safe_free(hmap->data);
  hmap->data = NULL;
}


bool array_hmap_good_key(array_hmap_pair_t *d){
  return d->key != ARRAY_HASH_EMPTY_KEY &&
	  d->key != ARRAY_HASH_DELETED_KEY;
}


/*
 * Make a copy of record d in a clean array data
 * - mask = size of data - 1 (size must be a power of 2)
 */
static void array_hmap_clean_copy(array_hmap_pair_t *data, array_hmap_pair_t *d, uint32_t mask) {
  uint32_t j;

  j = d->hash & mask;
  while (data[j].key != ARRAY_HASH_EMPTY_KEY) {
    j ++;
    j &= mask;
  }

  data[j].key = d->key;
  data[j].val = d->val;
  data[j].hash = d->hash;
  data[j].length = d->length;
}


/*
 * Remove deleted records
 */
static void array_hmap_cleanup(array_hmap_t *hmap) {
  array_hmap_pair_t *tmp, *d;
  uint32_t j, n, mask;

  n = hmap->size;
  tmp = (array_hmap_pair_t *) safe_malloc(n * sizeof(array_hmap_pair_t));
  for (j=0; j<n; j++) {
    tmp[j].key = ARRAY_HASH_EMPTY_KEY;
  }

  mask = n - 1;
  d = hmap->data;
  for (j=0; j<n; j++) {
    if (array_hmap_good_key(d)){
      array_hmap_clean_copy(tmp, d, mask);
    }
    d++;
  }

  safe_free(hmap->data);
  hmap->data = tmp;
  hmap->ndeleted = 0;
}


/*
 * Remove deleted records and make the table twice as large
 */
static void array_hmap_extend(array_hmap_t *hmap) {
  array_hmap_pair_t *tmp, *d;
  uint32_t j, n, n2, mask;

  n = hmap->size;
  n2 = n << 1;
  if (n2 >= ARRAY_HMAP_MAX_SIZE) {
    out_of_memory();
  }

  tmp = (array_hmap_pair_t *) safe_malloc(n2 * sizeof(array_hmap_pair_t));
  for (j=0; j<n2; j++) {
    tmp[j].key = ARRAY_HASH_EMPTY_KEY;
  }

  mask = n2 - 1;
  d = hmap->data;
  for (j=0; j<n; j++) {
    if (array_hmap_good_key(d)){
      array_hmap_clean_copy(tmp, d, mask);
    }
    d++;
  }

  safe_free(hmap->data);
  hmap->data = tmp;
  hmap->size = n2;
  hmap->ndeleted = 0;

  hmap->resize_threshold = (uint32_t)(n2 * ARRAY_HMAP_RESIZE_RATIO);
  hmap->cleanup_threshold = (uint32_t)(n2 * ARRAY_HMAP_CLEANUP_RATIO);
}


bool array_size_equal(int size, int32_t *a, int32_t *b){
  uint32_t i;
  for (i=0; i<size; i++){
    if (a[i] != b[i]){
      return false;
    }
  }
  return true;
}

bool array_term_equal(int32_t term, //terminator symbol
			    int32_t *a, int32_t *b){
  int i = 0;
  while(a[i]==b[i] && a[i] != term && b[i] != term){
    i++;
  }
  return (a[i] == b[i]);
}
  

/*
 * Find record with key k using array_size_equal
 * - return NULL if k is not in the table
 */
array_hmap_pair_t *array_size_hmap_find(array_hmap_t *hmap, int32_t size,
				   int32_t *k) {
  uint32_t mask, j, jhash;
  array_hmap_pair_t *d;


  jhash = jenkins_hash_intarray(size, k);
  mask = hmap->size - 1;
  j = jhash & mask;  
  for (;;) {
    d = hmap->data + j;
    if (d->key == ARRAY_HASH_EMPTY_KEY) return NULL;
    if (d->key != ARRAY_HASH_DELETED_KEY && 
	jhash == d->hash &&
	size == d->length &&
	array_size_equal(size, d->key, k))
      return d;
    j ++;
    j &= mask;
  }
}


/*
 * Add record with key k after hmap was extended:
 * - initialize val to -1
 * - we know that no record with key k is present
 */
static array_hmap_pair_t *array_size_hmap_get_clean(array_hmap_t *hmap,
					       int32_t size,
					       int32_t *k) {
  uint32_t mask, j, jhash;
  array_hmap_pair_t *d;

  mask = hmap->size - 1;
  jhash = jenkins_hash_intarray(size, k);
  j =  jhash & mask;
  for (;;) {
    d = hmap->data + j;
    if (!array_hmap_good_key(d)){
      hmap->nelems ++;
      d->key = k;
      d->val = -1;
      d->length = size;
      d->hash = jhash;
      return d;
    }
    j ++;
    j &= mask;
  }
}


/*
 * Find or add record with key k
 */
array_hmap_pair_t *array_size_hmap_get(array_hmap_t *hmap, int32_t size,
				       int32_t *k) {
  uint32_t mask, j, jhash;
  array_hmap_pair_t *d, *aux;

  assert(k >= 0);
  assert(hmap->size > hmap->ndeleted + hmap->nelems);

  mask = hmap->size - 1;
  jhash = jenkins_hash_intarray(size, k);
  j = jhash & mask;

  for (;;) {
    d = hmap->data + j;
    if (!array_hmap_good_key(d)) break;
    if (jhash == d->hash && size == d->length && array_size_equal(size, d->key, k))  return d;
    j ++;
    j &= mask;
  }

  aux = d; // new record, if needed, will be aux
  while (d->key != ARRAY_HASH_EMPTY_KEY) {
    j ++;
    j &= mask;
    d = hmap->data + j;
    if (jhash == d->hash && size == d->length && array_size_equal(size, d->key, k)) return d;
  }

  if (hmap->nelems + hmap->ndeleted >= hmap->resize_threshold) {
    array_hmap_extend(hmap);
    aux = array_size_hmap_get_clean(hmap,size, k);
  } else {
    hmap->nelems ++;
    aux->key = k;
    aux->val = -1;
    aux->hash = jhash;
    aux->length = size;
  }

  return aux;
}



/*
array_hmap_pair_t *array_term_hmap_find(array_hmap_t *hmap, 
				   int32_t *k) {
  uint32_t mask, j, jhash;
  array_hmap_pair_t *d;

  jhash = jenkins_hash_intarray(size, k);
  mask = hmap->size - 1;
  j = jhash & mask;  
  for (;;) {
    d = hmap->data + j;
    if (jhash == d->hash && array_term_equal(KEY_TERMINATOR, d->key, k)) return d;
    if (d->key == ARRAY_HASH_EMPTY_KEY) return NULL;
    j ++;
    j &= mask;
  }
}
*/


/*
 * Find or add record with key k
 */
/* array_hmap_pair_t *array_term_hmap_get(array_hmap_t *hmap, 
				   int32_t *k) {
   uint32_t mask, j, jhash;
  array_hmap_pair_t *d, *aux;

  assert(hmap->size > hmap->ndeleted + hmap->nelems);
  jhash = jenkins_hash_intarray(size, k);
  mask = hmap->size - 1;
  j = jhash & mask;

  for (;;) {
    d = hmap->data + j;
    if (array_term_equal(KEY_TERMINATOR, d->key, k)) return d;
    if (d->key < 0) break;
    j ++;
    j &= mask;
  }

  aux = d; // new record, if needed, will be aux
  while (d->key != ARRAY_HASH_EMPTY_KEY) {
    j ++;
    j &= mask;
    if (array_term_equal(KEY_TERMINATOR, d->key, k)) return d;
  }

  if (hmap->nelems + hmap->ndeleted >= hmap->resize_threshold) {
    array_hmap_extend(hmap);
    aux = array_hmap_get_clean(hmap,size, k);
  } else {
    hmap->nelems ++;
    aux->key = k;
    aux->val = -1;
    aux->hash = size
  }

  return aux;
}
*/
 
/*  Check for memory leak with the deletion of the key
 * Erase record r
 */
void array_hmap_erase(array_hmap_t *hmap, array_hmap_pair_t *r) {
  //  assert(array_hmap_find(hmap, r->key) == r);

  r->key = ARRAY_HASH_DELETED_KEY;
  hmap->nelems --;
  hmap->ndeleted ++;
  if (hmap->ndeleted > hmap->cleanup_threshold) {
    array_hmap_cleanup(hmap);
  }
}


/* Check for memory leak in the key deletion
 * Empty the map
 */
void array_hmap_reset(array_hmap_t *hmap) {
  uint32_t i, n;
  array_hmap_pair_t *d;

  n = hmap->size;
  d = hmap->data;
  for (i=0; i<n; i++) {
    d->key = ARRAY_HASH_EMPTY_KEY;
    d ++;
  }

  hmap->nelems = 0;
  hmap->ndeleted = 0;
}
