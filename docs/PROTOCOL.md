# Philips Hue OmniGlow Lightstrip protocol notes

**Status:** independent reverse engineering of one user-owned strip, recorded 2026-09-12. This document is a measured working description, not an official Philips specification.

## Evidence and confidence

| Statement | Status | Evidence or limit |
|---|---|---|
| Captured symbol period is approximately1.25µs | Measured at187.5ns sample spacing | Clean red/blue captures; original clock not independently calibrated |
| Generated high intervals are4 and15cycles; generated slot is20cycles | Implemented and instruction-checked | Accepted red/blue replay; original pulse tolerances remain unknown |
| An active frame is 6,448 symbols, or 806 bytes | Measured for the working replay | 80-symbol repetition and the 48-symbol tail; first symbol is inferred from alignment |
| The first 6,400 symbols are an 80-symbol pattern repeated 80 times | Measured pattern structure | Exact repetition in the clean red and blue snapshot sets |
| The final 48 symbols are a common footer | Measured pattern structure | Same tail in the red and blue message-part captures |
| The footer is the bytes `03 75 D7 5E 78 08` | Measured in the working byte representation | A checksum or CRC meaning is not proved |
| 80 units × 10 bytes × five 16-bit values | Inferred wire model | Fits the 806-byte frame and successful generated tests; unit meaning is unknown |
| Values are sent MSB first | Inferred/implemented convention | Used by the working serializer; channel semantics remain unconfirmed |
| Red/blue channel mapping | Inferred from user-labelled captures | Green has not been independently measured; white/CCT order is unconfirmed |
| Full red, full blue, 40/40 red-blue, and swapped 40/40 | User-confirmed behavior | Confirmed on the tested intact strip after the original DATA line was isolated |
| A universal 80-pixel or LED count | Unsupported | “80 units” is a frame grouping, not a confirmed physical LED count |
| Controller IC or official protocol identity | Unknown | No chip identification or Philips specification has been established |

Published evidence: [red aligned captures](../evidence/red-aligned-captures.json), [blue aligned captures](../evidence/blue-aligned-captures.json), [red reconstruction](../evidence/red-reconstruction.json), [blue reconstruction](../evidence/blue-reconstruction.json), and [compiled waveform checks](../evidence/waveform-verification.json). Each aligned window samples1024logic states at187.5ns spacing. Windows at different offsets belong to different transmissions; reconstruction relies on stable repeated data and checks overlapping symbols for conflicts. It is not a single continuous8ms capture.

## Electrical notes

The test setup used:

- an Arduino Uno / ATmega328P at 16 MHz;
- nominal 5 V Arduino logic;
- a strip powered from its nominal 24 V supply;
- a common ground between the tested signal source and strip;
- D2 as the Arduino DATA pin in the verified harness.

The strip-side input threshold, protection, isolation, and any level-shifter requirement were not measured. Keep 24 V away from Arduino signal pins. The original Hue controller DATA connection must be physically isolated before Arduino output is connected or a replay command is sent.

Wire colours are not a universal pinout. Verify DATA, GND and supply on the actual device.

## Captured timing and accepted replay timing

At 16 MHz, one CPU cycle is 62.5 ns. The compiled output and waveform reconstruction use:

| Quantity | Cycles | Time |
|---|---:|---:|
| Symbol slot | 20 | 1.25 µs |
| Short high | 4 | 250 ns |
| Long high | 15 | 937.5 ns |
| Working frame | 6,448 symbols | 8.06 ms |
| Repeated body | 6,400 symbols | 8.00 ms |
| Footer | 48 symbols | 60 µs |

A short high pulse is labeled `0` and a long high pulse is labeled `1` in analysis files. That is a measurement label, not proof of the device’s logical bit polarity.

The coarse envelope capture measured complete inter-frame low gaps from approximately 12.889 ms to 14.304 ms. The generated Arduino tests use `delay(13)` after each frame; the generated low interval additionally includes software rendering overhead. The captured original-controller gap is a separate measurement. A measured start-to-start period was approximately 20.8–22.1 ms in the coarse run. These are practical test timings, not a claimed timing tolerance for every strip.

## Working frame model

The current serializer writes 806 transmitted bytes:

```text
bytes 0..799   80 repeated units × 10 bytes
bytes 800..805 six-byte footer: 03 75 D7 5E 78 08
```

Each ten-byte unit is represented as five big-endian 16-bit values:

```text
value 0: byte 0, byte 1
value 1: byte 2, byte 3
value 2: byte 4, byte 5
value 3: byte 6, byte 7
value 4: byte 8, byte 9
```

This is a working representation. It should not be read as a confirmed Philips field map. The 80-unit grouping could describe receiver words or an internal packet layout rather than physical LEDs. The tested controller IC is unknown, and the captures do not establish a general LED count, address format, brightness law, checksum, or colour-space conversion.

The serializer keeps one extra byte as a non-transmitted sentinel so the final byte preload remains inside the buffer. The sentinel is not part of the frame.

## Observed values

Using the short-high/long-high convention and the inferred five-word grouping, the clean user-labelled captures produced these representative values:

| Capture label | Value 0 | Value 1 | Value 2 | Value 3 | Value 4 |
|---|---:|---:|---:|---:|---:|
| Red | `A3D6` | `00AD` | `0000` | `0234` | `0000` |
| Blue | `0000` | `25D6` | `7B54` | `0000` | `043A` |

An older blue capture differed in shade, so these values are examples from specific user-labelled states rather than fixed colour constants. Different blue selections yielded different payloads; no universal blue constant or calibrated color transform has been established.

The final 48 symbols were stable across the red and blue message-part captures. They are encoded in the working serializer as `03 75 D7 5E 78 08`. No evidence currently proves that those six bytes are a CRC, terminator, brightness field, or any other named protocol field.

## Generated colour limits

The smooth demo uses only the first three inferred channels for generated RGB-like targets. Each target channel is bounded to 30,000, the three-channel sum is 30,000, and the remaining two channels are zero. The initial measured states contain larger individual values, including approximately 41,942 for the red-labelled state, 31,572 for blue, and 9,686 for blue’s other active channel; interpolation stays between the current state and the bounded target.

The transition uses integer smoothstep interpolation over 101 points from each current state to a new target. The intermediate multiplication uses 64-bit arithmetic. This avoids arithmetic overflow in the easing calculation but does not prove that the inferred channel model is physically correct.

## Firmware behaviour

The current playground is intended for an Uno at 16 MHz. It starts with D2 as an input, waits for an explicit serial command at 115200 baud, writes a bounded sequence of frames, and returns D2 to input with a low output latch after stopping or completing.

The public command summary is:

| Command | Action |
|---|---|
| `D` | 12-stage smooth random-colour demo; each stage uses 101 interpolation steps and independent targets for the two 40-unit halves |
| `W` | White-channel experiment |
| `P` | Eight-effect playground |
| `X` | Stop between frames and release D2 to input |

The `D` demo checks for `X` between frames. The signal output itself remains interrupt-free for the fixed 8.06 ms frame, then the serial stop check runs during the low interval. A command is not an authorization to connect the original controller DATA line; that physical isolation must be completed first.

## Reproduction and review limits

The replay implementation was checked from compiled assembly, including:

- 6,448 symbols per frame;
- 20-cycle spacing for every adjacent symbol;
- 4/15-cycle high durations;
- correct all-zero, all-one, random, red, and blue payload simulations;
- safe final-byte sentinel preload;
- low final output level;
- output masking limited to D2.

These checks establish that the generated firmware emits the intended working waveform. They do not identify the proprietary protocol or guarantee operation on another Hue product, another strip revision, a different power supply, or an untested electrical interface.

## Next verification

A future single ESP32-C6 Zigbee prototype should be paired with a real Hue Bridge and checked for color, dimming, color temperature and power-cycle recovery. Only then should a second custom device be built. This is integration work, not driver-chip identification. Do not turn the 80-unit inference into a universal Philips Hue or WS2812-style protocol claim.

## Reference material

- [Espressif Zigbee color and color-temperature light endpoint](https://docs.espressif.com/projects/arduino-esp32/en/latest/zigbee/ep_color_dimmable_light.html): candidate future wireless interface; not implemented here.
- [Philips Hue Zigbee interoperability](https://developers.meethue.com/zigbee-3-0-support-in-hue-ecosystem/): custom-device pairing remains a hardware test.
- [Current full sketch](../firmware/omniglow_playground/omniglow_playground.ino) and [serial control tool](../tools/control.py).
