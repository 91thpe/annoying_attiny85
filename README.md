# annoying_attiny85

A hidden office noise maker. An ATtiny85 on a mini-breadboard plays one
short, hard-to-locate burst of beeps at random 5–15 minute intervals.
It runs from USB power and has no controls: power on means running.

Plain avr-gcc in PlatformIO, flashed through an Arduino Micro running
ArduinoISP.

## What it does

1. **Power-on:** silent for 10 minutes. The buzzer is held off from the
   first instant, so plugging it in never makes a sound.
2. **Activation:** one burst of 3–5 short beeps (~20 ms each, ~0.1 s in
   total) with small random timing variation, so it never sounds
   mechanical.
3. **Wait** a random 5–15 minutes, counted from the end of the burst, then
   repeat. Every interval is drawn fresh.

Between sounds the chip sleeps and wakes once a second to count. Its
timer is only ~±10% accurate, so "10 minutes" means 9–11. Every power-up
gives a different random sequence: the chip keeps a power-up counter in
its EEPROM and mixes in clock jitter.

The buzzer's ~2.3 kHz sits in the band where people are worst at
locating a sound, and the bursts are too short to turn your head toward.
Placement matters too: put it behind or inside something.

## Parts

| Part | Notes |
|------|-------|
| ATtiny85 (DIP-8) | Fuses are never changed by this project |
| TDB05LFPN active buzzer | 5 V, 30 mA, 2300 Hz, 2-pin, marked (+) |
| MPSA42 NPN transistor (marked A42) | Switches the buzzer; the pin can't drive 30 mA |
| 1 kΩ resistor | Transistor base |
| 2 × 15 kΩ resistors | RESET pull-up, buzzer pull-down |
| 100 nF ceramic capacitor | Recommended, across the chip's VCC and GND |
| USB breakout board | 5 V supply |
| Arduino Micro | Programmer only, not part of the final build |

## Wiring

ATtiny85 pinout (notch or dot at the left, pin 1 bottom-left):

```
        8   7   6   5
      ┌─┴───┴───┴───┴─┐
      ◖  ATtiny85     │
      └─┬───┬───┬───┬─┘
        1   2   3   4

   1 RESET   2 PB3   3 PB4   4 GND
   5 PB0     6 PB1   7 PB2   8 VCC
```

Final circuit:

```
    5 V ─────┬──────────────────────────┬──────────┐
             │                          │          │
           15 kΩ                        │         (+)
             │                    pin 8 (VCC)   TDB05LFPN
    pin 1 ───┘ (RESET)                             (−)
                                                   │
                                                   C
    pin 6 (PB1) ─┬──── 1 kΩ ──────────────────B  MPSA42
                 │                                 E
               15 kΩ                               │
                 │                                 │
    GND ─────────┴──────── pin 4 (GND) ────────────┘

    100 nF between pin 8 and pin 4, close to the chip (recommended)
    Pins 2, 3, 5 and 7 unconnected
```

| From | To | Part |
|------|----|------|
| Pin 8 | 5 V | wire |
| Pin 4 | GND | wire |
| Pin 1 | 5 V | 15 kΩ (keeps the chip out of reset) |
| Pin 6 | GND | 15 kΩ (keeps the buzzer off until the firmware runs) |
| Pin 6 | MPSA42 base | 1 kΩ |
| MPSA42 emitter | GND | wire |
| MPSA42 collector | buzzer (−) | wire |
| Buzzer (+) | 5 V | wire |

**MPSA42 legs:** with the flat face toward you and the legs down, they're
usually E – B – C from left to right. Check with a meter's diode test if
unsure: the base is the leg shared by both junctions.

There's no flyback diode. The MPSA42 is rated 300 V, far above any spike
this small buzzer can produce. A small diode across the buzzer (cathode to
5 V) does no harm if you have one.

**Power:** 5 V from the USB breakout. To turn it off, unplug USB or pull a
jumper wire. A jumper wire can bounce when you plug it in; that's harmless,
because each bounce just restarts the 10-minute quiet period.

## Programming

### One-time: turn the Arduino Micro into a programmer

ArduinoISP is included as its own PlatformIO project. With only the Micro
plugged in:

```
cd tools\arduinoisp
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e micro -t upload
cd ..\..
```

The output shows a few "redefined" warnings during the "Converting
ArduinoISP.ino" step. They're harmless: they come from PlatformIO's
.ino-to-C++ converter, not the real compile.

### Wire the Micro to the ATtiny85

Unplug USB while wiring.

| Micro | ATtiny85 pin |
|-------|--------------|
| D10 | 1 (RESET) |
| MO (MOSI) | 5 |
| MI (MISO) | 6 |
| SCK | 7 |
| 5V | 8 |
| GND | 4 |

On the Micro, the SPI lines are the pins marked MO / MI / SCK, not
D11–D13. No capacitor is needed on the Micro's RESET.

The buzzer circuit can stay connected while flashing. The buzzer may
crackle during programming because pin 6 doubles as a programming line.
If verification ever fails, unplug the 1 kΩ from pin 6 while flashing.

### Build and flash

From the repo folder. Replace `COM12` with the Micro's port
(`pio.exe device list` shows it):

```
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e attiny85 -t upload --upload-port COM12
```

Success ends with `... bytes of flash verified`,
`Fuses OK (E:FF, H:DF, L:E2)` and `[SUCCESS]`. Your fuse values may
differ; the firmware sets the 8 MHz clock itself and never writes fuses.

Flashing ends with a reset, so the 10-minute quiet period starts right
away. The Micro's 5 V keeps the chip running on the bench.

To check the connection without writing anything:

```
& "$env:USERPROFILE\.platformio\packages\tool-avrdude\avrdude.exe" -C "$env:USERPROFILE\.platformio\packages\tool-avrdude\avrdude.conf" -p t85 -c arduino -P COM12 -b 19200
```

Expect `Device signature = 0x1e930b (probably t85)`.

### Troubleshooting

| Symptom | Cause |
|---------|-------|
| `not in sync` / `programmer is not responding` | Wrong COM port, or `-c stk500v1` used instead of `-c arduino` (the Micro only answers once the PC sets DTR, which `-c arduino` does) |
| `Device signature = 0x000000` or `0xffffff` | Wiring between the Micro and the ATtiny85: check MO/MI (easy to swap) and that pin 1 is the right way round |
| `verification error` | The buzzer circuit is loading pin 6: unplug the 1 kΩ while flashing |
| `'pio' is not recognized` | Use the full path shown above, or open a terminal from the PlatformIO sidebar |

## Changing the behavior

All settings are in [`src/config.h`](src/config.h). Edit, then flash again.

| Setting | Default | What it does |
|---------|---------|--------------|
| `QUIET_PERIOD_S` | 600 | Silence after power-on, in seconds |
| `INTERVAL_MIN_S` / `INTERVAL_MAX_S` | 300 / 900 | Random wait between activations |
| `CRICKET_CHIRPS_MIN` / `MAX` | 1 / 1 | Bursts per activation |
| `CRICKET_SYL_MIN` / `MAX` | 3 / 5 | Beeps per burst |
| `CRICKET_SYL_ON_MS` | 20 | Beep length (shorter may be too weak on an active buzzer) |
| `CRICKET_SYL_OFF_MS` | 18 | Gap between beeps in a burst |
| `CRICKET_CHIRP_PERIOD_MS` | 400 | Burst start to burst start, when there's more than one |
| `TONE_MAX_MS` / `ACTIVATION_MAX_MS` | 500 / 3000 | Hard limits, enforced in code |
| `BUZZER_ACTIVE_LOW` | 0 | Set to 1 if you switch to a PNP high-side driver |

### Test build for tuning at the desk

`env:attiny85_test` skips the quiet period. It repeats rounds of: a
beep-length ladder (5, 10, 15, 20, 30 ms), then three activations 4 s
apart, starting 3 s after power-on.

```
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e attiny85_test -t upload --upload-port COM12
```

Don't leave the test build installed in the office. Flash the normal
build (`-e attiny85`) when you're done.

## Repository layout

```
platformio.ini        ATtiny85 builds: attiny85 (normal), attiny85_test
src/config.h          all settings
src/main.c            start-up, schedule, test mode
src/pattern.c/.h      burst generator + hard limits (no AVR code)
src/rng.c/.h          xorshift32, unbiased ranges, seed mixing (no AVR code)
src/sound.c/.h        buzzer pin driver
src/sleep.c/.h        watchdog sleep, clock-jitter entropy
test/                 host tests for rng and pattern: make -C test
tools/arduinoisp/     ArduinoISP for the Micro (unmodified Arduino example)
annoying_esp-lite/    separate ESP32-S3 version, see below
docs/HANDOFF.md       original brief
docs/DESIGN.md        design notes and decisions
```

The host tests run with a normal C compiler (`make -C test`), not on the
chip.

## ESP32 version (annoying_esp-lite)

A minimal Arduino-framework version for an M5Stamp S3: one 80 ms beep at
power-on to show it's running, then one beep every random 5–25 minutes.
It prints the countdown over USB.

- Wiring: GPIO 7 → 1 kΩ → NPN base, 15 kΩ from GPIO 7 to GND, emitter to
  GND, collector to buzzer (−), buzzer (+) to the Stamp's 5V pin.
- Settings: the constants at the top of `annoying_esp-lite/src/main.cpp`.
- Flash from that folder:
  `pio.exe run -t upload -t monitor --upload-port COM5 --monitor-port COM5`.
  If no port shows up, hold the Stamp's button while plugging in. Use a
  data-capable USB-C cable; many are power-only.
