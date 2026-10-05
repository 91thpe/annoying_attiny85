# Design notes (milestone 1)

Companion to [`HANDOFF.md`](HANDOFF.md). This file records the decisions,
the numbers behind them, and what still needs the owner's confirmation.
Anything marked **VERIFY** has not been tested on hardware yet.

## 1. Confirmed defaults

| Parameter | Value | Source |
|-----------|-------|--------|
| Quiet period after power-on | 600 s | Owner |
| Interval | uniform in [300, 900] s, counted from the end of the previous sound | Owner |
| Fuses | Factory default, never touched (8 MHz RC, CKDIV8 on, BOD off) | Owner |
| CPU clock | 8 MHz, set at runtime via `CLKPR` | Handoff §5.1 |
| Buzzer | Passive, 3-wire module, `BUZZER_PASSIVE = 1` | Owner (polarity: see §6) |
| Signal pin | PB1 / OC1A (DIP pin 6), Timer1 | Handoff §5.2 |
| First profile | Cricket-ish only | Handoff §4.2 |

## 2. Toolchain

- PlatformIO, `platform = atmelavr`, `board = attiny85`, no framework.
- **Change from the handoff:** the official PlatformIO example for
  Arduino as ISP uses `upload_protocol = custom` with an explicit avrdude
  command, not `upload_protocol = stk500v1`. Source:
  `platformio/platformio-docs`, `platforms/atmelavr_extra.rst` (develop
  branch). The `platformio.ini` in milestone 2 follows that example:

  ```ini
  upload_protocol = custom
  upload_port = COM3            ; owner fills in the Mega's port
  upload_speed = 19200
  upload_flags =
      -C
      ${platformio.packages_dir}/tool-avrdude/avrdude.conf
      -p
      $BOARD_MCU
      -P
      $UPLOAD_PORT
      -b
      $UPLOAD_SPEED
      -c
      stk500v1
  upload_command = avrdude $UPLOAD_FLAGS -U flash:w:$SOURCE:i
  ```

  **VERIFY** on the owner's PC: the `avrdude.conf` path depends on the
  installed `tool-avrdude` package version.
- `TEST_MODE` becomes a **second PlatformIO environment**
  (`[env:attiny85_test]` with `-DTEST_MODE=1`), so switching between
  builds doesn't mean editing source. The normal env never defines it.
  Everything else stays a constant in `src/config.h`.
- **Cloud sandbox:** `avr-gcc` 7.3.0 + `avr-libc` 2.0.0 install and compile
  for `-mmcu=attiny85`. The PlatformIO registry is blocked, so `pio run`
  can't run here. I'll compile with raw `avr-gcc` using the same flags before
  every push and report the size. The owner's `pio run` is the authoritative
  build.
- **Host tests** (PRNG, interval drawing, profile timing) use plain C with
  host `gcc` and a `Makefile` in `test/`. They run in the cloud sandbox. The
  owner doesn't need to run them (PlatformIO's `native` platform needs MinGW
  on Windows, which isn't worth setting up for this).

## 3. Tone generation on Timer1

ATtiny85 Timer1 runs from the system clock through a prescaler
N ∈ {1, 2, 4, …, 16384} (`CS13:0`), with `OCR1C` as TOP.

**Decision: use PWM mode (`PWM1A = 1`) for everything**, not CTC toggle.
One mode gives both pitch (`OCR1C`) and duty (`OCR1A`). A 50% duty is a
plain square wave, and fades become possible later without changing the
driver.

    f = F_CPU / (N · (OCR1C + 1)),   duty = OCR1A / (OCR1C + 1)

`COM1A1:0 = 10` (clear OC1A on compare match, set at BOTTOM) drives only
PB1. The complementary `/OC1A` output on PB0 stays off.

At 8 MHz the driver picks the smallest prescaler where TOP ≤ 255. That
keeps the best resolution:

| Target | N | OCR1C | Actual | Step size |
|--------|---|-------|--------|-----------|
| 500 Hz (sweep floor) | 64 | 249 | 500.0 Hz | 0.4% |
| 2.0 kHz | 16 | 249 | 2000 Hz | 0.4% |
| 2.7 kHz | 16 | 184 | 2703 Hz | 0.54% |
| 3.0 kHz | 16 | 166 | 2994 Hz | 0.6% |
| 4.0 kHz | 8 | 249 | 4000 Hz | 0.4% |
| 5.0 kHz (sweep ceiling) | 8 | 199 | 5000 Hz | 0.5% |

The ±2–4% pitch variation is therefore 4–8 register steps. That's enough.

The datasheet only guarantees the factory-calibrated RC oscillator to ±10%
(at 3 V, 25 °C). Real parts are usually much closer, but that isn't promised. Absolute pitch may be a few percent off. That doesn't matter: the
`TEST_MODE` sweep finds the loudest point empirically.

**Caveat on fades:** changing the duty cycle on a passive transducer
changes the fundamental's amplitude roughly with sin(π·D), but also the
harmonic content. So a duty ramp gives a usable soft onset, not a clean
amplitude envelope. That's fine for its purpose (removing the onset
transient). Not part of milestone 3.

**Stopping a tone:** clear `COM1A1:0` (disconnects the timer from the pin),
stop the clock (`CS13:0 = 0`), and drive PB1 to the idle level.

## 4. Timing and sleep

- **Long waits:** watchdog in interrupt mode, 8 s period, power-down sleep.
  Count ticks down. At ±10% watchdog tolerance, a "600 s" quiet period
  is 540–660 s. Acceptable.
- **Short waits during a sound:** `_delay_ms`/`_delay_us` busy waits.
  Profile timings are all ≥ 1 ms, and nothing else runs during a sound.
- **Hard caps** are enforced in code, not just by the tables: each tone is
  clamped to 500 ms, and the activation is aborted at 3 s.

## 5. Randomness

Seed = mix of:

1. **EEPROM boot counter** (32-bit, incremented every power-up). That
   alone guarantees a different sequence after every power cycle. EEPROM
   endurance (100k writes) is irrelevant at this usage.
2. **Clock jitter:** run Timer0 free-running from the CPU clock and
   sample `TCNT0` at each of ~32 watchdog interrupts (64 ms each, so
   ~2 s inside the quiet period, costs nothing). The CPU stays awake
   (idle sleep or busy wait) during sampling, because power-down stops
   Timer0. The watchdog's 128 kHz
   oscillator and the 8 MHz RC oscillator are independent, so the low bits
   drift. How much entropy this gives isn't known in advance; the boot
   counter is the guaranteed part.
3. Optional: ADC noise from the internal temperature sensor. Skip unless (1)
   and (2) turn out to be visibly insufficient.

PRNG: xorshift32 (state never zero). Ranges are drawn by rejection
sampling: reject draws ≥ the largest multiple of the range, then take
`%`. All of this goes in host-testable code.

## 6. Open item: buzzer trigger polarity is not yet proven

The handoff says the module is "high-triggered" because it clicks when SIG
goes to 5 V. **That test doesn't show which level energizes the coil.**
A passive transducer clicks on *any* change in current, so a
low-triggered module also clicks at that edge (the coil switching off).

This matters because many 3-pin passive modules drive the transducer
through a **PNP transistor (often marked S8550 or 2TY), which makes them
active-LOW**. If this module is active-LOW, the planned 10 kΩ pull-down and
"idle LOW" firmware would hold the coil **on** permanently: constant
current, a warm buzzer, and no sound.

Two quick checks, either one is enough:

- **Read the transistor marking** on the module (SOT-23, three legs).
  `S8550`, `2TY`, or `9012` = PNP → active-LOW. `S8050`, `J3Y`, or `9013`
  = NPN → active-HIGH. No transistor at all (SIG goes straight to the
  transducer through a resistor) → active-HIGH.
- **Measure current:** meter in series with the module's VCC lead, 5 V
  supply. Tie SIG to GND and read, then tie SIG to 5 V and read. The state
  with tens of mA is "on". The other should read close to 0 mA.

Depending on the result:

| Result | Pull resistor on SIG | Idle level | Config |
|--------|---------------------|------------|--------|
| Active-HIGH | 10 kΩ to GND | LOW | `BUZZER_ACTIVE_LOW 0` |
| Active-LOW | 10 kΩ to 5 V | HIGH | `BUZZER_ACTIVE_LOW 1` |

The firmware supports both through one constant, but the resistor on the
breadboard has to match.

## 7. Smaller risks (no action needed now)

- **No brown-out detection** (fuses unchanged). A jumper that bounces or a
  slow 5 V ramp could start the chip in a bad state. The 10 kΩ reset
  pull-up, 100 nF decoupling, and 10 µF bulk capacitor mitigate this. If
  the device ever runs erratically after power-on, BOD at 2.7 V
  (`BODLEVEL = 101`) is the one fuse change worth considering. It's safe
  (doesn't affect ISP), but it's the owner's call.
- **ISP speed:** the chip runs at 1 MHz until the firmware sets `CLKPR`.
  ArduinoISP's default SPI clock is slow enough for a 1 MHz target.
  **VERIFY** in milestone 2.
- **PB1 is MISO during programming.** Program on the Mega rig, then move
  the chip (or unplug SIG while flashing), as the handoff says.

## 8. Milestone plan

1. Design notes (this file). Waiting for review.
2. "Hello": `platformio.ini`, one 2.7 kHz beep every 2 s on PB1 via
   Timer1. Flash over the Mega. Confirms toolchain, ISP rig, clock, pin,
   and polarity.
3. Cricket-ish profile + sweep in `TEST_MODE`.
4. Full schedule build (quiet period, random intervals, seeding,
   sleep).
5. README (ASCII wiring, programming, build/flash, tuning).
