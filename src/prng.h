/*
 * Simple PRNG based on linear congruence modulo 2^32.
 *
 * Recurrence X_{t+1} = (a X_t + b) mod 2^32
 * - X_0 is the seed,
 * - a = 1664525,
 * - b = 1013904223
 * (Source: Wikipedia + Knuth's Art of Computer Programming, Vol. 2)
 *
 * Low-order bits are not random.
 *
 * Note: the global state of the PRNG (variable seed) is local.
 * So every file that imports this will have its own copy of the PRNG,
 * and all copies have the same default seed.
 */

#ifndef __PRNG_H
#define __PRNG_H

#include <stdint.h>

#define prng_multiplier 1664525
#define prng_constant   1013904223

static uint32_t seed = 0xabcdef98; // default seed

static inline void random_seed(uint32_t s) {
  seed = s;
}

static inline uint32_t random_uint32() {
  uint32_t x;
  x = seed;
  seed = seed * ((uint32_t) prng_multiplier) + ((uint32_t) prng_constant);
  return x;
}

static inline int32_t random_int32() {
  return (int32_t) random_uint32();
}

// random integer between 0 and n-1 (remove 8 low-order bits)
static inline uint32_t random_uint(uint32_t n) {
  return (random_uint32() >> 8) % n;  
}

/*
 * Other linear congruence
 */
static double dseed = 91648253;

// Returns a random float 0 <= x < 1. Seed must never be 0.
static inline double drand() {
  int q;
  dseed *= 1389796;
  q = (int)(dseed / 2147483647);
  dseed -= (double)q * 2147483647;
  return dseed / 2147483647; 
}

// Returns a random integer 0 <= x < size. Seed must never be 0.
static inline int irand(int size) {
  return (int)(drand() * size); 
}


#endif


