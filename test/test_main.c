/*
 * Host tests for the AVR-independent code: rng.c and pattern.c.
 * Build and run: make -C test
 */
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "pattern.h"
#include "rng.h"

static int failures;

#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        failures++; \
    } \
} while (0)

static void test_xorshift_reference(void)
{
    /* Marsaglia xorshift32 (13/17/5) from seed 1. */
    rng_seed(1);
    CHECK(rng_next() == 270369UL);
    CHECK(rng_next() == 67634689UL);
    CHECK(rng_next() == 2647435461UL);
}

static void test_zero_seed(void)
{
    rng_seed(0);
    for (int i = 0; i < 1000; i++)
        CHECK(rng_next() != 0);
}

static void test_range_bounds(void)
{
    rng_seed(12345);
    for (int i = 0; i < 100000; i++) {
        uint32_t v = rng_range(300, 900);
        CHECK(v >= 300 && v <= 900);
        if (v < 300 || v > 900)
            return;
    }
    CHECK(rng_range(7, 7) == 7);
    CHECK(rng_range(9, 3) == 9);
    (void)rng_range(0, UINT32_MAX);     /* full range must not hang */
}

static void test_range_uniform(void)
{
    /* Chi-square over 10 buckets, 100k draws. 9 degrees of freedom:
     * p = 0.001 critical value is 27.9. */
    enum { K = 10, N = 100000 };
    long count[K] = { 0 };
    rng_seed(987654321);
    for (int i = 0; i < N; i++)
        count[rng_range(0, K - 1)]++;
    double chi = 0, e = (double)N / K;
    for (int k = 0; k < K; k++)
        chi += (count[k] - e) * (count[k] - e) / e;
    CHECK(chi < 27.9);
}

static void test_jitter(void)
{
    rng_seed(42);
    int seen_lo = 0, seen_hi = 0;
    for (int i = 0; i < 10000; i++) {
        uint16_t v = rng_jitter(15, 2);
        CHECK(v >= 13 && v <= 17);
        seen_lo |= v == 13;
        seen_hi |= v == 17;
    }
    CHECK(seen_lo && seen_hi);
    for (int i = 0; i < 1000; i++)
        CHECK(rng_jitter(1, 5) <= 6);   /* clamps at 0, no wraparound */
}

static void test_cricket_shape(void)
{
    int min_chirps = 99, max_chirps = 0;

    for (uint32_t seed = 1; seed <= 20000; seed++) {
        pattern_t p;
        rng_seed(seed);
        pattern_cricket(&p);

        CHECK(p.n >= CRICKET_CHIRPS_MIN * CRICKET_SYL_MIN);
        CHECK(p.n <= CRICKET_CHIRPS_MAX * CRICKET_SYL_MAX);
        CHECK(pattern_total_ms(&p) <= ACTIVATION_MAX_MS);
        CHECK(p.step[p.n - 1].off_ms == 0);

        int chirps = 1;
        for (uint8_t i = 0; i < p.n; i++) {
            const step_t *s = &p.step[i];
            CHECK(s->on_ms >= CRICKET_SYL_ON_MS - CRICKET_SYL_ON_JIT_MS);
            CHECK(s->on_ms <= CRICKET_SYL_ON_MS + CRICKET_SYL_ON_JIT_MS);
            CHECK(s->on_ms <= TONE_MAX_MS);
            /* a long gap marks the end of a chirp */
            if (i + 1 < p.n && s->off_ms > 100)
                chirps++;
        }
        if (chirps < min_chirps) min_chirps = chirps;
        if (chirps > max_chirps) max_chirps = chirps;

        /* The handoff's target: roughly 0.2-1.5 s per activation. */
        uint32_t t = pattern_total_ms(&p);
        CHECK(t >= 60 && t <= 1500);
    }
    CHECK(min_chirps == CRICKET_CHIRPS_MIN);
    CHECK(max_chirps == CRICKET_CHIRPS_MAX);
}

static void test_limit(void)
{
    pattern_t p = { 0 };

    /* One over-long beep is clamped. */
    p.n = 1;
    p.step[0] = (step_t){ 2000, 100 };
    pattern_limit(&p);
    CHECK(p.step[0].on_ms == TONE_MAX_MS);
    CHECK(p.step[0].off_ms == 0);

    /* 10 x (400 on + 400 off) = 8 s is cut to exactly the cap. */
    p.n = 10;
    for (int i = 0; i < 10; i++)
        p.step[i] = (step_t){ 400, 400 };
    pattern_limit(&p);
    CHECK(pattern_total_ms(&p) <= ACTIVATION_MAX_MS);
    CHECK(p.n == 4);
    CHECK(p.step[p.n - 1].off_ms == 0);
    CHECK(p.step[p.n - 1].on_ms > 0);

    /* Empty pattern stays empty. */
    p.n = 0;
    pattern_limit(&p);
    CHECK(p.n == 0);
}

int main(void)
{
    test_xorshift_reference();
    test_zero_seed();
    test_range_bounds();
    test_range_uniform();
    test_jitter();
    test_cricket_shape();
    test_limit();

    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
