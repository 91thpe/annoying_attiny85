/*
 * All tunable constants. Change here, rebuild, reflash.
 * This header is plain C (no AVR headers) so host tests can use it.
 */
#ifndef CONFIG_H
#define CONFIG_H

/* ---- Hardware ---------------------------------------------------------- */

/* Buzzer drive pin: PB1 (DIP pin 6), through an MPSA42 low-side switch. */
#define BUZZER_BIT          1

/* 0: pin HIGH = buzzer on (NPN low-side, current wiring).
 * 1: pin LOW  = buzzer on (PNP high-side). */
#define BUZZER_ACTIVE_LOW   0

/* ---- Schedule (normal build) ------------------------------------------ */

#define QUIET_PERIOD_S      600     /* silence after power-on */
#define INTERVAL_MIN_S      300     /* gap between activations, counted */
#define INTERVAL_MAX_S      900     /*   from the end of the previous one */

/* ---- Hard limits (enforced in code, whatever the tables say) ----------- */

#define TONE_MAX_MS         500     /* longest single beep */
#define ACTIVATION_MAX_MS   3000    /* longest whole activation */

/* ---- Cricket-ish profile ----------------------------------------------- */
/* An activation is 1-3 chirps; a chirp is 3-5 short beeps ("syllables").
 * Each value gets random +/- jitter per beep so it never sounds mechanical. */

#define CRICKET_CHIRPS_MIN      1
#define CRICKET_CHIRPS_MAX      3
#define CRICKET_SYL_MIN         3       /* syllables per chirp */
#define CRICKET_SYL_MAX         5
#define CRICKET_SYL_ON_MS       20      /* beep length (untested below 20:
                                           active buzzers start slowly) */
#define CRICKET_SYL_ON_JIT_MS   2
#define CRICKET_SYL_OFF_MS      18      /* gap between beeps in a chirp */
#define CRICKET_SYL_OFF_JIT_MS  3
#define CRICKET_CHIRP_PERIOD_MS 400     /* chirp start to chirp start */
#define CRICKET_CHIRP_PER_JIT_MS 40

/* ---- TEST_MODE (env:attiny85_test only) -------------------------------- */

#define TEST_START_DELAY_MS     3000    /* after power-on */
#define TEST_LADDER_GAP_MS      700     /* between ladder beeps */
#define TEST_CRICKET_REPEATS    3       /* cricket activations per round */
#define TEST_CRICKET_GAP_MS     4000    /* between them */
#define TEST_ROUND_GAP_MS       8000    /* before the next round */
/* The ladder itself is in main.c: 5, 10, 15, 20, 30 ms beeps. */

#endif
