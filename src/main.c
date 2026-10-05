/*
 * Normal build (env:attiny85): milestone 2 "hello" beep every 2 s, until the
 * real schedule arrives in milestone 4.
 *
 * Test build (env:attiny85_test, TEST_MODE=1), repeating rounds of:
 *   1. beep-length ladder: 5, 10, 15, 20, 30 ms, to find the shortest beep
 *      this buzzer still plays properly;
 *   2. TEST_CRICKET_REPEATS cricket-ish activations, each randomized.
 */
#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/power.h>

#include "config.h"
#include "pattern.h"
#include "rng.h"
#include "sound.h"

#ifndef TEST_MODE
#define TEST_MODE 0
#endif

/* Boot counter: a different random sequence after every power-up.
 * (Clock-jitter entropy is added in milestone 4.) */
static uint32_t EEMEM ee_boot_count;

static void seed_from_boot_count(void)
{
    uint32_t n = eeprom_read_dword(&ee_boot_count) + 1;
    eeprom_update_dword(&ee_boot_count, n);
    rng_seed(n * 2654435761UL);     /* spread small counts over all bits */
}

#if TEST_MODE
static const uint16_t ladder_ms[] = { 5, 10, 15, 20, 30 };

static void test_round(void)
{
    for (uint8_t i = 0; i < sizeof ladder_ms / sizeof ladder_ms[0]; i++) {
        sound_beep(ladder_ms[i]);
        delay_ms(TEST_LADDER_GAP_MS);
    }
    delay_ms(TEST_CRICKET_GAP_MS);

    for (uint8_t i = 0; i < TEST_CRICKET_REPEATS; i++) {
        pattern_t p;
        pattern_cricket(&p);
        sound_play(&p);
        delay_ms(TEST_CRICKET_GAP_MS);
    }
}
#endif

int main(void)
{
    sound_init();

    /* Run at 8 MHz (F_CPU) whatever CKDIV8 says: a factory chip boots at
     * 8 MHz / 8 = 1 MHz. avr-libc does the timed CLKPR sequence in asm. */
    clock_prescale_set(clock_div_1);

    seed_from_boot_count();

#if TEST_MODE
    delay_ms(TEST_START_DELAY_MS);
    for (;;) {
        test_round();
        delay_ms(TEST_ROUND_GAP_MS);
    }
#else
    for (;;) {
        sound_beep(HELLO_BEEP_MS);
        delay_ms(HELLO_PERIOD_MS - HELLO_BEEP_MS);
    }
#endif
}
