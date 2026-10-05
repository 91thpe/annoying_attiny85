/*
 * Milestone 2 "hello": one short beep every 2 s on PB1.
 * Confirms toolchain, ISP rig, clock switch, pin and drive polarity.
 */
#include <avr/io.h>
#include <avr/power.h>
#include <util/delay.h>

#include "config.h"

static void buzzer_off(void)
{
#if BUZZER_ACTIVE_LOW
    PORTB |= _BV(BUZZER_PIN);
#else
    PORTB &= ~_BV(BUZZER_PIN);
#endif
}

static void buzzer_on(void)
{
#if BUZZER_ACTIVE_LOW
    PORTB &= ~_BV(BUZZER_PIN);
#else
    PORTB |= _BV(BUZZER_PIN);
#endif
}

int main(void)
{
    /* Set the idle level before the pin becomes an output, so it never
     * glitches to the "on" level. */
    buzzer_off();
    DDRB |= _BV(BUZZER_PIN);

    /* Factory fuses give 8 MHz / 8 = 1 MHz. Drop the divider to run at
     * 8 MHz (F_CPU). avr-libc does the timed CLKPR sequence in asm. */
    clock_prescale_set(clock_div_1);

    for (;;) {
        buzzer_on();
        _delay_ms(HELLO_BEEP_MS);
        buzzer_off();
        _delay_ms(HELLO_PERIOD_MS - HELLO_BEEP_MS);
    }
}
