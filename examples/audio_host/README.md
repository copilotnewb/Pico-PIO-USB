# Standalone DeskHop audio-host diagnostics

This target is a **diagnostic test**, not DeskHop keyboard/mouse firmware and not
an established fix for missing audio. It keeps TinyUSB's upstream Audio Host
application and the fork's existing PIO transfer implementation. It adds evidence
at their boundaries instead of changing USB scheduling, handshakes or retries.

## Build and flash

Use the existing repository build workflow, or a Pico SDK checkout whose TinyUSB
contains the Audio Host driver and example:

```sh
cmake -S examples -B examples/build -DPICO_BOARD=pico
cmake --build examples/build --target audio_host
```

Flash `examples/build/audio_host/audio_host.uf2` to the Pico connected to the
DeskHop USB-A socket being tested. Use `pico`, not `pico2`, for an RP2040 Pico.
The CI artifact is named `example_binaries-pico-sdk-<version>` and also contains
other examples; select its `audio_host` binary, not the HID/CDC binary.

Host wiring remains D+=GP14 and D-=GP15, clock 120 MHz, TinyUSB rhport 1. Endpoint
capacity remains 384 bytes; input/output audio buffers remain 256/384 bytes.
The boot banner prints actual pin settings, clock and the TinyUSB commit built.
CI still follows TinyUSB master, so save that revision with every test result.

## Serial output

For the Pico/Pico2 boards in CI, UART0 TX is GP0 and RX is GP1. Connect Pico GP0
to a **3.3 V USB-UART adapter's RX**, plus a shared GND. This test only requires
receiving logs; leave the adapter TX and power/VCC pins disconnected. Power the
board normally. Do not use 5 V TTL or RS-232 signalling.

Settings: **921600 baud, 8 data bits, no parity, 1 stop bit, no flow control**.
Logging is UART, **not a USB CDC port on the Pico's native USB connector**.

Connect the UART before booting to capture enumeration. The log reports:

- HCD attach/remove, reset start/end, each control SETUP submission.
- Received device/configuration descriptors and Audio Host parser acceptance.
- Audio mount, every endpoint's attributes, maximum packet and open result.
- Upstream format selection and asynchronous audio start/stop/failure events.
- Once-per-second PIO frame progress, normalized line state, connection state,
  ISO OUT submissions and audio completion totals.

`SETUP` fields are USB request values in hex, except length. For example,
`type=01 req=0b` is SET_INTERFACE; `type=22 req=01` is a UAC1 endpoint SET_CUR.
A SETUP line proves **submission**, not successful completion. Device/config
callbacks, start-complete events and playback callbacks provide later evidence.

`frame_delta` counts calls to the PIO frame handler, not proof that SOF packets
reached the device. `line=FS` is a sampled full-speed idle state. GPIO input
inversion is intentional in this library; the observer uses its normalized
line-state helper. A snapshot is not an electrical bus trace.

No printing occurs in the HCD event IRQ or for each ISO packet. UART printing
still takes time in the main loop: use this build to localize failures, not to
benchmark uninterrupted audio performance.

## LED without serial

Count short flashes, followed by a longer gap. The code is the furthest
successful milestone **since the last observed attach/remove**, not a live
streaming indicator. The normal upstream heartbeat is replaced.

| Flashes | Evidence obtained |
| --- | --- |
| 1 | Diagnostic main loop alive; no HCD attach observed yet |
| 2 | HCD attach reached TinyUSB's event hook |
| 3 | Complete device descriptor received |
| 4 | Complete configuration descriptor received |
| 5 | Audio Host accepted/parses the audio function |
| 6 | Audio mount callback ran |
| 7 | Playback start completed successfully |
| 8 | At least two playback completion callbacks occurred |
| 9 | Initialization, endpoint submission or audio failure observed |

Nine flashes overrides the progress code. UART retains the last successful
stage and failure messages. A successful subsequent playback start clears the
fault indication. Capture-only success does not count as playback success.
Stage 8 means the host reported transmissions: ISO OUT has no ACK, so it cannot
prove that the headset accepted samples or produced audible sound. Use the
increasing `SPK_CB` total to establish continued progress, not stage 8 alone.

## First hardware run

Start with the playback-only UGREEN/KTMicro adapter, with working headphones
connected. Record the boot banner, all enumeration lines, and several status
lines. Then repeat with the Nova 7 dongle. Allow the upstream application to
cycle: capture-capable devices start with roughly five seconds of microphone
capture, then five seconds of speaker sine, then five seconds of echo. A
playback-only device starts the sine immediately after successful configuration.
These are application phases, not evidence the corresponding hardware works.

Interpret the last boundary rather than changing several variables:

| Last evidence | Next area to investigate |
| --- | --- |
| No advancing frame count | PIO alarm/frame scheduling and initialization |
| Frames advance, SE0 and no attach | Data connection, connector/pins, pull-up/power |
| Attach/reset but no device descriptor | Reset and EP0 transactions; inspect last SETUP |
| Device descriptor but no configuration | Later EP0 requests, including string descriptors |
| Configuration but no audio parser acceptance | Actual descriptor topology/parser compatibility |
| Audio parsed/mounted but no playback start | Format choice, endpoint open, SET_INTERFACE/SET_CUR |
| Start but no repeated completions | Submission/PIO scheduling/completion path |
| Increasing completions but no sound | On-wire packet validity/cadence, controls, output routing |

These are investigation directions, not diagnoses. No hardware result has been
inferred from successful compilation.

## Implementation and tests

Only this example wraps HCD reset, SETUP, endpoint-open and endpoint-transfer
calls. Each wrapper invokes the original once and returns its result. Only
`audio_app.c` receives callback-renaming macros; observers forward to those
renamed callbacks so upstream configuration and restart behavior remain intact.
TinyUSB currently gives transfer-completion hook events rhport 0 even for PIO
USB; link events are filtered to rhport 1, while transfers are counted for this
single-host-controller application.

Run the hardware-independent state/LED regression test from the repository root:

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Iexamples/audio_host tests/test_audio_diagnostics.c -o /tmp/test_audio_diagnostics
/tmp/test_audio_diagnostics
```

It checks monotonic milestones, replug reset, repeated-completion detection,
counter saturation and every LED pattern. It does not simulate USB signalling.
