/*
 * Buzzer driver (AVR only): active buzzer switched on PB1.
 */
#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>
#include "pattern.h"

/* Drive the pin to its "off" level, then make it an output. Call first. */
void sound_init(void);

/* Blocking: play every step of p. */
void sound_play(const pattern_t *p);

/* Blocking: one beep of on_ms (clamped to TONE_MAX_MS). */
void sound_beep(uint16_t on_ms);

/* Blocking delay with a run-time length. */
void delay_ms(uint32_t ms);

#endif
