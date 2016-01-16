/*
 * Hash map from integers
 */

#include <assert.h>
#include <stdbool.h>
#include "memalloc.h"
#include "hash_map.h"
#include "hash_functions.h"
#include "utils.h"

/*
 * Initialization:
 * - n = initial size, must be a power of 2
 * - if n = 0, the default size is used
 */
void init_hmap(hmap_t *hmap, uint32_t n) {
	uint32_t i;
	hmap_pair_t *tmp;
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
		n = HMAP_DEFAULT_SIZE;
	}

	if (n >= HMAP_MAX_SIZE) {
		out_of_memory();
	}

	tmp = (hmap_pair_t *) safe_malloc(n * sizeof(hmap_pair_t));
	for (i = 0; i < n; i++) {
		tmp[i].key = HASH_EMPTY_KEY;
	}

	hmap->data = tmp;
	hmap->size = n;
	hmap->nelems = 0;
	hmap->ndeleted = 0;

	hmap->resize_threshold = (uint32_t)(n * HMAP_RESIZE_RATIO);
	hmap->cleanup_threshold = (uint32_t) (n * HMAP_CLEANUP_RATIO);
}


/*
 * Make a faithful full copy of the hmap (from ==> to):
 */
void copy_hmap(hmap_t *to, hmap_t *from) {
  int32_t n;

  n = from->size;
  to->size = n;

  to->data = (hmap_pair_t *) safe_malloc(n * sizeof(hmap_pair_t));
  memcpy(to->data, from->data, n*sizeof(hmap_pair_t));

  to->nelems = from->nelems;
  to->ndeleted = from->ndeleted;
  to->resize_threshold = from->resize_threshold;
  to->cleanup_threshold = from->cleanup_threshold;
}




/*
 * Free memory
 */
void delete_hmap(hmap_t *hmap) {
	safe_free(hmap->data);
	hmap->data = NULL;
}


bool hmap_good_key(hmap_pair_t *d){
	return d->key != HASH_EMPTY_KEY &&
		d->key != HASH_DELETED_KEY;
}


/*
 * Make a copy of record d in a clean data
 * - mask = size of data - 1 (size must be a power of 2)
 */
static void hmap_clean_copy(hmap_pair_t *data, hmap_pair_t *d, uint32_t mask) {
	uint32_t j;

	j = d->hash & mask;
	while (data[j].key != HASH_EMPTY_KEY) {
		j++;
		j &= mask;
	}

	/* first empty position after the hash value */
	data[j].key = d->key;
	data[j].val = d->val;
	data[j].hash = d->hash;
}


/*
 * Remove deleted records
 */
static void hmap_cleanup(hmap_t *hmap) {
	hmap_pair_t *tmp, *d;
	uint32_t j, n, mask;

	n = hmap->size;
	tmp = (hmap_pair_t *) safe_malloc(n * sizeof(hmap_pair_t));
	for (j=0; j<n; j++) {
		tmp[j].key = HASH_EMPTY_KEY;
	}

	mask = n - 1;
	d = hmap->data;
	for (j=0; j<n; j++) {
		if (hmap_good_key(d)){
			hmap_clean_copy(tmp, d, mask);
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
static void hmap_extend(hmap_t *hmap) {
	hmap_pair_t *tmp, *d;
	uint32_t j, n, n2, mask;

	n = hmap->size;
	n2 = n << 1;
	if (n2 >= HMAP_MAX_SIZE) {
		out_of_memory();
	}

	tmp = (hmap_pair_t *) safe_malloc(n2 * sizeof(hmap_pair_t));
	for (j=0; j<n2; j++) {
		tmp[j].key = HASH_EMPTY_KEY;
	}

	mask = n2 - 1;
	d = hmap->data;
	for (j=0; j<n; j++) {
		if (hmap_good_key(d)){
			hmap_clean_copy(tmp, d, mask);
		}
		d++;
	}

	safe_free(hmap->data);
	hmap->data = tmp;
	hmap->size = n2;
	hmap->ndeleted = 0;

	hmap->resize_threshold = (uint32_t)(n2 * HMAP_RESIZE_RATIO);
	hmap->cleanup_threshold = (uint32_t)(n2 * HMAP_CLEANUP_RATIO);
}

bool term_equal(int32_t term, //terminator symbol
		int32_t *a, int32_t *b){
	int i = 0;
	while(a[i]==b[i] && a[i] != term && b[i] != term){
		i++;
	}
	return (a[i] == b[i]);
}


/*
 * Find record with key k using size_equal
 * - return NULL if k is not in the table
 */
hmap_pair_t *hmap_find(hmap_t *hmap, int32_t k) {
	uint32_t mask, j, jhash;
	hmap_pair_t *d;


	jhash = jenkins_hash_intarray(1, &k);
	mask = hmap->size - 1;
	j = jhash & mask;  
	for (;;) {
		d = hmap->data + j;
		if (d->key == HASH_EMPTY_KEY) return NULL;
		if (d->key != HASH_DELETED_KEY && 
				jhash == d->hash &&
				d->key == k)
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
static hmap_pair_t *hmap_get_clean(hmap_t *hmap,
		int32_t k) {
	uint32_t mask, j, jhash;
	hmap_pair_t *d;

	mask = hmap->size - 1;
	jhash = jenkins_hash_intarray(1, &k);
	j =  jhash & mask;
	for (;;) {
		d = hmap->data + j;
		if (!hmap_good_key(d)){
			hmap->nelems ++;
			d->key = k;
			d->val = -1;
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
hmap_pair_t *hmap_get(hmap_t *hmap, int32_t k) {
	uint32_t mask, j, jhash;
	hmap_pair_t *d, *aux;

	assert(k >= 0);
	assert(hmap->size > hmap->ndeleted + hmap->nelems);

	mask = hmap->size - 1;
	jhash = jenkins_hash_intarray(1, &k);
	j = jhash & mask;

	for (;;) {
		d = hmap->data + j;
		if (!hmap_good_key(d)) break;
		if (jhash == d->hash && d->key == k)  return d;
		j ++;
		j &= mask;
	}

	aux = d; // new record, if needed, will be aux
	while (d->key != HASH_EMPTY_KEY) {
		j ++;
		j &= mask;
		d = hmap->data + j;
		if (jhash == d->hash && d->key == k) return d;
	}

	if (hmap->nelems + hmap->ndeleted >= hmap->resize_threshold) {
		hmap_extend(hmap);
		aux = hmap_get_clean(hmap, k);
	} else {
		hmap->nelems ++;
		aux->key = k;
		aux->val = -1;
		aux->hash = jhash;
	}

	return aux;
}

/*  
 * Check for memory leak with the deletion of the key
 * Erase record r
 */
void hmap_erase(hmap_t *hmap, hmap_pair_t *r) {
	//  assert(hmap_find(hmap, r->key) == r);

	r->key = HASH_DELETED_KEY;
	hmap->nelems --;
	hmap->ndeleted ++;
	if (hmap->ndeleted > hmap->cleanup_threshold) {
		hmap_cleanup(hmap);
	}
}


/*
 * Check for memory leak in the key deletion
 * Empty the map
 */
void hmap_reset(hmap_t *hmap) {
	uint32_t i, n;
	hmap_pair_t *d;

	n = hmap->size;
	d = hmap->data;
	for (i=0; i<n; i++) {
		d->key = HASH_EMPTY_KEY;
		d ++;
	}

	hmap->nelems = 0;
	hmap->ndeleted = 0;
}

/* 
 * Remove a random element from the hash table 
 *
 * TODO can be done more efficiently
 */ 
int32_t hmap_remove_random(hmap_t *hmap) {
	int32_t index = genrand_uint(hmap->nelems);
	hmap_pair_t *d = hmap->data;
	for (; index >= 0; index--) {
		while (!hmap_good_key(d)) {
			d++;
		}
	}
	int32_t key = d->key;
	hmap_erase(hmap, d);
	return key;
}

