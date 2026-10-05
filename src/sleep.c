#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "sleep.h"

static volatile uint8_t wdt_fired;

ISR(WDT_vect)
{
    wdt_fired = 1;
}

/* Watchdog in interrupt-only mode (never resets the chip). */
static void wdt_interrupt_mode(uint8_t prescale_bits)
{
    cli();
    MCUSR &= ~_BV(WDRF);
    wdt_reset();
    WDTCR = _BV(WDCE) | _BV(WDE);           /* timed sequence: 4 cycles */
    WDTCR = _BV(WDIE) | prescale_bits;
    sei();
}

static void wdt_stop(void)
{
    cli();
    wdt_reset();
    MCUSR &= ~_BV(WDRF);
    WDTCR = _BV(WDCE) | _BV(WDE);
    WDTCR = 0;
    sei();
}

static void sleep_until_wdt(uint8_t mode)
{
    wdt_fired = 0;
    while (!wdt_fired) {
        set_sleep_mode(mode);
        cli();
        if (!wdt_fired) {
            sleep_enable();
            sei();                  /* sei+sleep are atomic on AVR */
            sleep_cpu();
            sleep_disable();
        }
        sei();
    }
}

void sleep_seconds(uint32_t s)
{
    wdt_interrupt_mode(_BV(WDP2) | _BV(WDP1));      /* ~1 s */
    while (s--)
        sleep_until_wdt(SLEEP_MODE_PWR_DOWN);
    wdt_stop();
}

uint32_t sleep_jitter_entropy(void)
{
    uint32_t acc = 0;

    TCCR0A = 0;
    TCCR0B = _BV(CS00);                     /* Timer0 free-running at 8 MHz */
    wdt_interrupt_mode(0);                  /* ~16 ms */

    for (uint8_t i = 0; i < 32; i++) {
        sleep_until_wdt(SLEEP_MODE_IDLE);   /* idle keeps Timer0 running */
        acc = (acc << 3 | acc >> 29) ^ TCNT0;
    }

    wdt_stop();
    TCCR0B = 0;
    return acc;
}
