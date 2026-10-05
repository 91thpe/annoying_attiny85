# Design notes (milestone 1)

Companion to [`HANDOFF.md`](HANDOFF.md). This file records the decisions,
the numbers behind them, and what still needs the owner's confirmation.
Anything marked **VERIFY** has not been tested on hardware yet.

## 1. Confirmed defaults

| Parameter | Value | Source |
|-----------|-------|--------|
| Quiet period after power-on | 600 s | Owner |
| Interval | uniform in [300, 900] s, counted from the end of the previous sound | Owner |
| Fuses | Never written by this project. The owner's chip reads **L:E2 H:DF E:FF** (8 MHz RC, CKDIV8 already *off*, BOD off), so it was set up for 8 MHz before. The runtime `CLKPR` write handles both cases | Owner; read on the bench |
| CPU clock | 8 MHz, set at runtime via `CLKPR` | Handoff §5.1 |
| Buzzer | DB Products TDB05LFPN: bare 2-pin active magnetic buzzer, 5 V, 30 mA, 2300 Hz, 85 dBA. `BUZZER_PASSIVE = 0` | Owner; specs from distributor listings (see §6) |
| Signal pin | PB1 (DIP pin 6), plain on/off GPIO | Handoff §5.2 |
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

> **Parked.** The owner switched to an active buzzer, so the firmware only
> switches PB1 on and off. This section stays as the reference for the
> `BUZZER_PASSIVE = 1` path if a passive piezo is used later. PB1 is kept
> as the signal pin so that path stays open.

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

## 6. Buzzer: active TDB05LFPN, driven through an NPN

The first passive transducer turned out to have no driver (SIG wired
straight to the transducer). Rather than settle piezo vs coil, the owner
switched to an **active buzzer**. Consequences:

**What changes in the sound design**

- Pitch is fixed by the buzzer (typically ~2.3–2.7 kHz for a 12 mm part,
  which happens to sit in the hard-to-locate band). No ±pitch variation, no
  Tweet sweep, no `TEST_MODE` frequency sweep.
- Variation comes from timing only: number of chirps, syllables per chirp,
  on/off lengths, small jitter on each.
- Profiles that still work: Cricket-ish (rhythm only), Lone chirp, Tick.
- **Start-up time:** an active buzzer's oscillator needs a few ms to reach
  full volume. 15 ms syllables may come out softer or smeared, and 1–2 ms
  ticks may be inaudible. `TEST_MODE` will play a ladder of syllable lengths
  (e.g. 5, 10, 15, 20, 30 ms) to find the shortest one that's clearly
  audible.

**The part:** DB Products TDB05LFPN (also sold as Jameco ValuePro). Per
distributor listings (Jameco, Octopart; no manufacturer datasheet read):
bare 2-pin active **magnetic** buzzer, 5 V rated (4–7 V range), **30 mA**,
**2300 Hz**, 85 dBA. 2.3 kHz is inside the hard-to-locate band.

**Drive: low-side NPN, not straight from the pin.** 30 mA is under the
pin's 40 mA absolute maximum but three times the 10 mA the output voltage
is specified at, and it would run for the life of the device.

Transistor: the owner's TO-92 parts marked **A42** = MPSA42 (B331 is a lot
/ date code). High-voltage NPN, but fine as a 30 mA switch: hFE ≥ 40 at
30 mA, V_CE(sat) ≤ 0.5 V at 20 mA / 2 mA base, 500 mA max
([EIC datasheet](https://datasheet.lcsc.com/lcsc/2204021730_EIC-Semicon-MPSA42_C2978815.pdf),
[onsemi](https://www.mouser.com/datasheet/2/149/MPSA42-196155.pdf)).
Preferred over the 2N3906 because it keeps the logic active-HIGH, which
matches the original plan.

    5 V ──────────────────────┐
                              │
                             (+)
                          TDB05LFPN
                             (−)
                              │
                              C
    PB1 (pin 6) ─┬── 1 kΩ ──B   MPSA42 (NPN)
                 │            E
               15 kΩ          │
                 │            │
    GND ─────────┴────────────┘

- PB1 **HIGH → buzzer on**, PB1 LOW → off. `BUZZER_ACTIVE_LOW 0`, idle
  level LOW.
- 1 kΩ base resistor: ≈ 4.3 mA base current, a forced gain of ~7 at
  30 mA. Comfortably saturated even at the datasheet's minimum hFE.
- 15 kΩ (anything 10–47 kΩ works) from PB1 to GND keeps the transistor off from reset until the
  firmware runs.
- Buzzer sees about 5 V − 0.2…0.5 V ≈ 4.5–4.8 V, inside its 4–7 V range.
- **No flyback diode** (owner has none). Acceptable: the MPSA42 is rated
  300 V V_CEO, far above any spike a 12 mm buzzer coil can produce. Add a
  small diode across the buzzer (cathode to 5 V) if one turns up.
- **Pinout:** MPSA42 in TO-92 is usually E-B-C (flat face toward you, legs
  down, left to right), but check the maker's datasheet or a meter's diode
  test (base is the common pin of both junctions). Mind the buzzer's (+)
  marking.

Fallback: the 2N3906 as a high-side switch also works, but inverts the
logic (LOW = on).

## 6b. Programmer: Arduino Micro, not the Mega

The owner flashes with an **Arduino Micro** (ATmega32U4, 5 V) running
ArduinoISP instead of the Mega. Differences from the handoff's §8:

| Micro | ATtiny85 pin |
|-------|--------------|
| MO (MOSI) | 5 (PB0) |
| MI (MISO) | 6 (PB1) |
| SCK | 7 (PB2) |
| D10 (target reset, per ArduinoISP) | 1 (PB5/RESET) |
| 5V | 8 (VCC) |
| GND | 4 (GND) |

- The SPI lines are the pins marked MO / MI / SCK (or the 6-pin ICSP
  header), **not** D11–D13.
- **No 10 µF capacitor on the Micro's RESET.** The 32U4 has native USB and
  doesn't auto-reset when the port opens (only on a 1200-baud "touch").
- **Use `-c arduino`, not `-c stk500v1`.** The 32U4's USB serial only
  transmits once the host asserts DTR. avrdude 6.3 on Windows with
  `stk500v1` leaves DTR off, so the Micro never answers ("not in sync").
  `-c arduino` sets DTR. On the 32U4 that doesn't reset the board (only a
  1200-baud touch does). Arduino IDE's "Arduino as ISP (ATmega32U4)"
  programmer uses the same protocol. Found on the owner's bench.
- The Micro's COM port number can differ from the Mega's, and changes
  while the Micro is in its bootloader. Use the port it shows while running
  ArduinoISP.

## 7. Smaller risks (no action needed now)

- **No brown-out detection** (fuses unchanged). A jumper that bounces or a
  slow 5 V ramp could start the chip in a bad state. The 15 kΩ reset
  pull-up, 100 nF decoupling, and 10 µF bulk capacitor mitigate this. If
  the device ever runs erratically after power-on, BOD at 2.7 V
  (`BODLEVEL = 101`) is the one fuse change worth considering. It's safe
  (doesn't affect ISP), but it's the owner's call.
- **ISP speed:** the chip runs at 1 MHz until the firmware sets `CLKPR`.
  ArduinoISP's default `SPI_CLOCK` is 1 MHz / 6, chosen for exactly this
  case (confirmed in the sketch source in `tools/arduinoisp/`).
- **PB1 is MISO during programming.** Program on the Mega rig, then move
  the chip (or unplug SIG while flashing), as the handoff says.

## 8. Milestone plan

1. Design notes (this file). Done.
2. "Hello": `platformio.ini`, one short beep every 2 s on PB1. **Done**
   (owner heard a beep every 2 s through the MPSA42). Caveat: this chip
   already had CKDIV8 off, so the test does not prove the `CLKPR` write on
   a factory-fresh chip. Flash over the Mega. Confirms toolchain, ISP rig, clock, pin,
   and polarity.
3. Cricket-ish profile + syllable-length ladder in `TEST_MODE`.
4. Full schedule build (quiet period, random intervals, seeding,
   sleep).
5. README (ASCII wiring, programming, build/flash, tuning).
