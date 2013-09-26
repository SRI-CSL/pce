/*
 * Hash map from integers
 */

#ifndef __HASH_MAP_H
#define __HASH_MAP_H

#include <stdint.h>

/*
 * Records stored in the hash table are pairs of key and value
 */
typedef struct hmap_pair_s {
  int32_t key; //key is an integer
  int32_t val; //mapped value of the key
  uint32_t hash;//hash value for key
} hmap_pair_t; 

/*
 * Markers for empty/deleted pairs
 */
#define  HASH_DELETED_KEY (INT32_MIN + 0)
#define  HASH_EMPTY_KEY (INT32_MIN + 1)

typedef struct hmap_s {
  hmap_pair_t *data;
  uint32_t size; // must be a power of 2
  uint32_t nelems;
  uint32_t ndeleted;
  uint32_t resize_threshold;
  uint32_t cleanup_threshold;
} hmap_t;


/*
 * Default initial size
 */
#define HMAP_DEFAULT_SIZE 32
#define HMAP_MAX_SIZE (UINT32_MAX/8)

/*
 * Ratios: resize_threshold = size * RESIZE_RATIO
 *         cleanup_threshold = size * CLEANUP_RATIO
 */
#define HMAP_RESIZE_RATIO 0.6
#define HMAP_CLEANUP_RATIO 0.2

/*
 * Initialization:
 * - n = initial size, must be a power of 2
 * - if n = 0, the default size is used
 */
extern void init_hmap(hmap_t *hmap, uint32_t n);

/*
 * Delete: free memory
 */
extern void delete_hmap(hmap_t *hmap);

/*
 * Find record with key k. Return NULL if there's none
 */
extern hmap_pair_t *hmap_find(hmap_t *hmap, int32_t k);

/*
 * Get record with key k. If one is in the table return it.
 * Otherwise, add a fresh record with key k and value -1 and return it.
 */
extern hmap_pair_t *hmap_get(hmap_t *hmap, int32_t k);

/*
 * Find record with key k with terminator KEY_TERMINATOR. Return NULL if there's none
 */

#define KEY_TERMINATOR 0;

/*
 * Erase record r
 */
extern void hmap_erase(hmap_t *hmap, hmap_pair_t *r);

/*
 * Remove all records
 */
extern void hmap_reset(hmap_t *hmap);

/* Remove a random element of the hash map */
extern int32_t hmap_remove_random(hmap_t *hmap);

#endif /* __HASH_MAP_H */
