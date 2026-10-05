#include "rng.h"

static uint32_t state = 2463534242UL;

void rng_seed(uint32_t seed)
{
    state = seed ? seed : 2463534242UL;
}

uint32_t rng_mix32(uint32_t x)
{
    /* MurmurHash3 fmix32 finalizer */
    x ^= x >> 16;
    x *= 0x85ebca6bUL;
    x ^= x >> 13;
    x *= 0xc2b2ae35UL;
    x ^= x >> 16;
    return x;
}

uint32_t rng_next(void)
{
    /* Marsaglia xorshift32, shifts 13/17/5 */
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
}

uint32_t rng_range(uint32_t lo, uint32_t hi)
{
    if (hi <= lo)
        return lo;

    uint32_t span = hi - lo + 1;
    if (span == 0)                      /* full 32-bit range */
        return rng_next();

    /* Reject the lowest (2^32 mod span) values so every residue is equally
     * likely. (0 - span) % span == 2^32 mod span in uint32 arithmetic. */
    uint32_t threshold = (0U - span) % span;
    uint32_t x;
    do {
        x = rng_next();
    } while (x < threshold);
    return lo + x % span;
}

uint16_t rng_jitter(uint16_t base, uint16_t jit)
{
    uint16_t lo = base > jit ? base - jit : 0;
    return (uint16_t)rng_range(lo, (uint32_t)base + jit);
}
