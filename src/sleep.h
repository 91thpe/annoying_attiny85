/*
 * Long waits (watchdog interrupt + power-down sleep) and seeding entropy.
 * AVR only.
 */
#ifndef SLEEP_H
#define SLEEP_H

#include <stdint.h>

/* Sleep about `s` seconds in power-down, waking once per second on the
 * watchdog. The watchdog oscillator is only ~+/-10% accurate. */
void sleep_seconds(uint32_t s);

/* Entropy from the drift between the watchdog's 128 kHz oscillator and the
 * CPU's 8 MHz RC oscillator: samples Timer0 at 32 watchdog interrupts
 * (16 ms each, ~0.5 s). Quality is unmeasured; the boot counter is the part
 * that guarantees a new sequence per power-up. */
uint32_t sleep_jitter_entropy(void);

#endif
