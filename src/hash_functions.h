/*
 * Hash functions: all return an unsigned 32bit integer
 */

#ifndef __HASH_FUNCTIONS_H
#define __HASH_FUNCTIONS_H

#include <stdint.h>


/*
 * CORE HASH FUNCTIONS
 */

/*
 * Hash of a null-terminated array of bytes (unsigned char, terminated by '\0')
 */
extern uint32_t jenkins_hash_byte_var(const uint8_t *s, uint32_t seed);


/*
 * Hash of an array of n integers 
 */
extern uint32_t jenkins_hash_array(const uint32_t *d, uint32_t n, uint32_t seed);


/*
 * Hash code for an arbitrary pointer
 */
extern uint32_t jenkins_hash_ptr(void *p);


/*
 * Hash code for a 32bit integer
 */
extern uint32_t jenkins_hash_uint32(uint32_t x);


/*
 * Hash code for a 64bit integer
 */
extern uint32_t jenkins_hash_uint64(uint64_t x);



/*
 * Hash functions for pairs, triple, and 4-tuples of integers.
 */
extern uint32_t jenkins_hash_pair(uint32_t a, uint32_t b, uint32_t seed);
extern uint32_t jenkins_hash_triple(uint32_t a, uint32_t b, uint32_t c, uint32_t seed);
extern uint32_t jenkins_hash_quad(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t seed);


/*
 * Mix two or three hash codes
 */
extern uint32_t jenkins_hash_mix2(uint32_t x, uint32_t y);
extern uint32_t jenkins_hash_mix3(uint32_t x, uint32_t y, uint32_t z);




/*
 * VARIANTS
 */

/*
 * Hash of null-terminated string s, using a default seed.
 */
static inline uint32_t jenkins_hash_string(const char * s) {
  return jenkins_hash_byte_var((const uint8_t *) s, 0x17838abc);
}

/*
 * Hash of an array of signed integers, default seed,
 */
static inline uint32_t jenkins_hash_intarray(const int32_t *d, uint32_t n) {
  return jenkins_hash_array((const uint32_t *) d, n, 0x17836abc);
}

/*
 * Array of signed integers, user-provided seed
 */
static inline uint32_t jenkins_hash_intarray2(const int32_t *d, uint32_t n, uint32_t seed) {
  return jenkins_hash_array((const uint32_t *) d, n, seed);
}


/*
 * Signed 32 or 64 bit integers
 */
static inline uint32_t jenkins_hash_int32(int32_t x) {
  return jenkins_hash_uint32(x);
}

static inline uint32_t jenkins_hash_int64(int64_t x) {
  return jenkins_hash_uint64(x);
}



#endif
