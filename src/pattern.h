/*
 * Sound patterns as lists of (on, off) steps in milliseconds.
 * Generating them is plain C (host-tested); playing them is in sound.c.
 */
#ifndef PATTERN_H
#define PATTERN_H

#include <stdint.h>

typedef struct {
    uint16_t on_ms;     /* buzzer on */
    uint16_t off_ms;    /* silence after it */
} step_t;

/* Enough for 3 chirps x 5 syllables, with room to spare. */
#define PATTERN_MAX_STEPS 24

typedef struct {
    uint8_t n;
    step_t step[PATTERN_MAX_STEPS];
} pattern_t;

/* Build one cricket-ish activation, randomized with rng.c. */
void pattern_cricket(pattern_t *p);

/* Enforce the hard limits from config.h: clamp each beep to TONE_MAX_MS
 * and cut the pattern so the whole thing lasts at most ACTIVATION_MAX_MS.
 * The last step's trailing silence is dropped (it isn't part of the sound). */
void pattern_limit(pattern_t *p);

/* Total duration in ms, including gaps. */
uint32_t pattern_total_ms(const pattern_t *p);

#endif
