/*
 * Hash map from integer arrays to integers for maintaining atoms
 */

#ifndef __ARRAY_HASH_MAP_H
#define __ARRAY_HASH_MAP_H

#include <stdint.h>

/*
 * Records stored in the hash table are pairs of array and integer
 * - key is array
 */
typedef struct array_hmap_pair_s {
  int32_t *key; //key is an array of ints
  int32_t val; //mapped value of the key
  uint32_t hash;//hash value for key
  int32_t length; //length of the array
} array_hmap_pair_t; 

/*
 * Markers for empty/deleted pairs
 */

#define  ARRAY_HASH_DELETED_KEY  ((int32_t *) 1)
#define  ARRAY_HASH_EMPTY_KEY NULL

typedef struct array_hmap_s {
  array_hmap_pair_t *data;
  uint32_t size; // must be a power of 2
  uint32_t nelems;
  uint32_t ndeleted;
  uint32_t resize_threshold;
  uint32_t cleanup_threshold;
} array_hmap_t;


/*
 * Default initial size
 */
#define ARRAY_HMAP_DEFAULT_SIZE 32
#define ARRAY_HMAP_MAX_SIZE (UINT32_MAX/8)

/*
 * Ratios: resize_threshold = size * RESIZE_RATIO
 *         cleanup_threshold = size * CLEANUP_RATIO
 */
#define ARRAY_HMAP_RESIZE_RATIO 0.6
#define ARRAY_HMAP_CLEANUP_RATIO 0.2

/*
 * Initialization:
 * - n = initial size, must be a power of 2
 * - if n = 0, the default size is used
 */
extern void init_array_hmap(array_hmap_t *hmap, uint32_t n);

/*
 * Delete: free memory
 */
extern void delete_array_hmap(array_hmap_t *hmap);

/*
 * Find record with key k. Return NULL if there's none
 */
extern array_hmap_pair_t *array_size_hmap_find(array_hmap_t *hmap,
					  int32_t size,
					  int32_t *k);

/*
 * Get record with key k. If one is in the table return it.
 * Otherwise, add a fresh record with key k and value -1 and return it.
 */
extern array_hmap_pair_t *array_size_hmap_get(array_hmap_t *hmap,
					 int32_t size,
					 int32_t *k);

/*
 * Find record with key k with terminator KEY_TERMINATOR. Return NULL if there's none
 */

#define KEY_TERMINATOR 0;

/*extern array_hmap_pair_t *array_term_hmap_find(array_hmap_t *hmap,
					  int32_t size,
					  int32_t *k);
*/

/*
 * Get record with key k with terminator KEY_TERMINATOR. If one is in the table return it.
 * Otherwise, add a fresh record with key k and value -1 and return it.
 */
/*extern array_hmap_pair_t *array_term_hmap_get(array_hmap_t *hmap,
					 int32_t size,
					 int32_t *k);
*/


/*
 * Erase record r
 */
extern void array_hmap_erase(array_hmap_t *hmap, array_hmap_pair_t *r);

/*
 * Remove all records
 */
extern void array_hmap_reset(array_hmap_t *hmap);


#endif /* __ARRAY_HASH_MAP_H */
