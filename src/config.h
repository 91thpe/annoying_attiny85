/*
 * All tunable constants. Change here, rebuild, reflash.
 */
#ifndef CONFIG_H
#define CONFIG_H

/* ---- Hardware ---------------------------------------------------------- */

/* Buzzer drive pin: PB1 (DIP pin 6), through an MPSA42 low-side switch. */
#define BUZZER_PIN          PB1

/* 0: pin HIGH = buzzer on (NPN low-side, current wiring).
 * 1: pin LOW  = buzzer on (PNP high-side). */
#define BUZZER_ACTIVE_LOW   0

/* ---- Milestone 2 "hello" ----------------------------------------------- */

#define HELLO_BEEP_MS       100     /* beep length */
#define HELLO_PERIOD_MS     2000    /* beep start to beep start */

#endif
