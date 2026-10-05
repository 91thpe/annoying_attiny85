/*
 * M5Stamp S3 office beeper.
 *
 * Power-on: one beep to show it's alive. Then, forever: wait a random
 * 5-25 minutes, beep once.
 *
 * Wiring: GPIO -> 1 kOhm -> NPN base. Emitter -> GND.
 *         Collector -> buzzer (-). Buzzer (+) -> Stamp 5V pin.
 *         15 kOhm from GPIO to GND keeps the buzzer off while booting.
 */
#include <Arduino.h>

const int BUZZER_PIN = 7;                   // the GPIO wired to the 1 kOhm
const uint32_t BEEP_MS = 80;                // one short beep
const uint32_t INTERVAL_MIN_S = 5 * 60;
const uint32_t INTERVAL_MAX_S = 25 * 60;

static void beep(uint32_t ms)
{
    digitalWrite(BUZZER_PIN, HIGH);
    delay(ms);
    digitalWrite(BUZZER_PIN, LOW);
}

void setup()
{
    digitalWrite(BUZZER_PIN, LOW);          // set "off" before driving the pin
    pinMode(BUZZER_PIN, OUTPUT);
    Serial.begin(115200);

    delay(500);
    beep(BEEP_MS);                          // "I'm running"
}

void loop()
{
    // random(a, b) returns a..b-1; on the ESP32 it draws from the hardware RNG
    uint32_t wait_s = random(INTERVAL_MIN_S, INTERVAL_MAX_S + 1);
    Serial.printf("next beep in %lu s (%lu min %lu s)\n",
                  (unsigned long)wait_s, (unsigned long)(wait_s / 60),
                  (unsigned long)(wait_s % 60));
    delay(wait_s * 1000UL);
    beep(BEEP_MS);
}
