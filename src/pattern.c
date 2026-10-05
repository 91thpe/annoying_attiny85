#include "pattern.h"
#include "config.h"
#include "rng.h"

static void add(pattern_t *p, uint16_t on_ms, uint16_t off_ms)
{
    if (p->n < PATTERN_MAX_STEPS) {
        p->step[p->n].on_ms = on_ms;
        p->step[p->n].off_ms = off_ms;
        p->n++;
    }
}

void pattern_cricket(pattern_t *p)
{
    p->n = 0;
    uint8_t chirps = rng_range(CRICKET_CHIRPS_MIN, CRICKET_CHIRPS_MAX);

    for (uint8_t c = 0; c < chirps; c++) {
        uint8_t syl = rng_range(CRICKET_SYL_MIN, CRICKET_SYL_MAX);
        uint16_t chirp_ms = 0;

        for (uint8_t s = 0; s < syl; s++) {
            uint16_t on = rng_jitter(CRICKET_SYL_ON_MS, CRICKET_SYL_ON_JIT_MS);
            uint16_t off = rng_jitter(CRICKET_SYL_OFF_MS, CRICKET_SYL_OFF_JIT_MS);
            if (on == 0)
                on = 1;
            add(p, on, off);
            chirp_ms += on + off;
        }

        /* Stretch the last gap so the next chirp starts one period later. */
        uint16_t period = rng_jitter(CRICKET_CHIRP_PERIOD_MS,
                                     CRICKET_CHIRP_PER_JIT_MS);
        if (p->n && period > chirp_ms)
            p->step[p->n - 1].off_ms += period - chirp_ms;
    }

    pattern_limit(p);
}

void pattern_limit(pattern_t *p)
{
    uint32_t t = 0;

    for (uint8_t i = 0; i < p->n; i++) {
        step_t *st = &p->step[i];
        if (st->on_ms > TONE_MAX_MS)
            st->on_ms = TONE_MAX_MS;

        if (t + st->on_ms > ACTIVATION_MAX_MS) {
            /* This beep would run past the cap: shorten it or drop it. */
            uint32_t room = ACTIVATION_MAX_MS - t;
            if (room == 0) {
                p->n = i;
                break;
            }
            st->on_ms = (uint16_t)room;
            st->off_ms = 0;
            p->n = i + 1;
            break;
        }
        t += st->on_ms;

        if (t + st->off_ms > ACTIVATION_MAX_MS)
            st->off_ms = (uint16_t)(ACTIVATION_MAX_MS - t);
        t += st->off_ms;
    }

    if (p->n)
        p->step[p->n - 1].off_ms = 0;
}

uint32_t pattern_total_ms(const pattern_t *p)
{
    uint32_t t = 0;
    for (uint8_t i = 0; i < p->n; i++)
        t += (uint32_t)p->step[i].on_ms + p->step[i].off_ms;
    return t;
}
