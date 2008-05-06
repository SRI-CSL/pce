/*
 * Hash functions: all return an unsigned 32bit integer
 */

#ifndef __HASH_FUNCTIONS_H
#define __HASH_FUNCTIONS_H 1

#include <stdint.h>

/*
 * Generic hash functions for (small) integers, using the Jenkin's mix function
 */

// hash with a seed
extern uint32_t jenkins_hash_pair(int32_t a, int32_t b, uint32_t seed);
extern uint32_t jenkins_hash_triple(int32_t a, int32_t b, int32_t c, uint32_t seed);
extern uint32_t jenkins_hash_quad(int32_t a, int32_t b, int32_t c, int32_t d, uint32_t seed);

// mix of two or three hash codes
extern uint32_t jenkins_hash_mix2(uint32_t x, uint32_t y);
extern uint32_t jenkins_hash_mix3(uint32_t x, uint32_t y, uint32_t z);

// null-termninated string
extern uint32_t jenkins_hash_string_ori(char *s, uint32_t seed);
extern uint32_t jenkins_hash_string_var(char *s, uint32_t seed);

// array of n integers
extern uint32_t jenkins_hash_intarray_ori(int n, int32_t *d, uint32_t seed);
extern uint32_t jenkins_hash_intarray_var(int n, int32_t *d, uint32_t seed);


/*
 * Use default seeds
 */
static inline uint32_t jenkins_hash_string(char * s) {
  return jenkins_hash_string_var(s, 0x17838abc);
}

static inline uint32_t jenkins_hash_intarray(int n, int32_t *d) {
  return jenkins_hash_intarray_var(n, d, 0x17836abc);
}



/*
 * Simpler hashing: faster but usually more collisions.
 */
extern uint32_t hash_string(char *s);
extern uint32_t hash_intarray(int n, int32_t *d);

static inline uint32_t hash_mix3(uint32_t x, uint32_t y, uint32_t z) {
  return (29 * x) ^ (31 * y) ^ (37 * z);
}

static inline uint32_t hash_mix(uint32_t x, uint32_t y) {
  return (23 * x) ^ (31 * y);
}


// constant hash for debugging hashtables
static inline uint32_t bad_hash_string(char *s) {
  return 27;
}


#endif
