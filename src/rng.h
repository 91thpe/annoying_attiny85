/*
 * Small PRNG: xorshift32 plus unbiased range draws.
 * Plain C, no AVR dependencies (host-tested in test/).
 */
#ifndef RNG_H
#define RNG_H

#include <stdint.h>

/* Seed the generator. A zero seed is replaced (xorshift must not be 0). */
void rng_seed(uint32_t seed);

/* Mix a 32-bit value so nearby inputs give unrelated outputs (seeding). */
uint32_t rng_mix32(uint32_t x);

/* Next raw 32-bit value. */
uint32_t rng_next(void);

/* Uniform integer in [lo, hi], inclusive, without modulo bias.
 * Returns lo if hi <= lo. */
uint32_t rng_range(uint32_t lo, uint32_t hi);

/* base +/- jit, uniform. Never below 0. */
uint16_t rng_jitter(uint16_t base, uint16_t jit);

#endif
