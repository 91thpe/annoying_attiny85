#include <avr/io.h>
#include <util/delay.h>

#include "config.h"
#include "sound.h"

static void buzzer_off(void)
{
#if BUZZER_ACTIVE_LOW
    PORTB |= _BV(BUZZER_BIT);
#else
    PORTB &= ~_BV(BUZZER_BIT);
#endif
}

static void buzzer_on(void)
{
#if BUZZER_ACTIVE_LOW
    PORTB &= ~_BV(BUZZER_BIT);
#else
    PORTB |= _BV(BUZZER_BIT);
#endif
}

void sound_init(void)
{
    /* Set the idle level before the pin becomes an output, so it never
     * glitches to the "on" level. */
    buzzer_off();
    DDRB |= _BV(BUZZER_BIT);
}

void delay_ms(uint32_t ms)
{
    /* _delay_ms() needs a compile-time constant; loop 1 ms at a time.
     * Loop overhead is a few cycles per ms at 8 MHz: negligible. */
    while (ms--)
        _delay_ms(1);
}

void sound_beep(uint16_t on_ms)
{
    if (on_ms > TONE_MAX_MS)
        on_ms = TONE_MAX_MS;
    buzzer_on();
    delay_ms(on_ms);
    buzzer_off();
}

void sound_play(const pattern_t *p)
{
    for (uint8_t i = 0; i < p->n; i++) {
        sound_beep(p->step[i].on_ms);
        delay_ms(p->step[i].off_ms);
    }
    buzzer_off();
}
