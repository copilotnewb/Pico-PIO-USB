# Audio Host CI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a buildable RP2040 Pico-PIO-USB TinyUSB Audio Host example and CI coverage that compiles the isochronous path with 256-byte endpoint buffers.

**Architecture:** Reuse TinyUSB's current upstream `examples/host/audio_host/src/audio_app.c` from the TinyUSB checkout already used by CI. Keep Pico-PIO-USB-specific glue local: a PIO host `main.c`, `tusb_config.h`, a tiny BSP compatibility header, and CMake integration. Existing examples remain unchanged.

**Tech Stack:** C11, Pico SDK, TinyUSB host stack, Pico-PIO-USB HCD, CMake, GitHub Actions.

**Spec:** User requested a real `audio_host` test/build on the fork after enabling Actions.

## Global Constraints

- Work directly on `main` as explicitly authorized by the repository owner.
- Keep `PIO_USB_EP_SIZE` default behavior unchanged for existing examples.
- Build the audio example with `PIO_USB_EP_SIZE=256`.
- Use TinyUSB's current Audio Host driver rather than vendoring or reimplementing UAC.
- Preserve existing Pico/Pico2 example builds.

---

### Task 1: Add a failing audio-host build target

**Files:**
- Modify: `examples/CMakeLists.txt`
- Create: `examples/audio_host/CMakeLists.txt`

- [ ] Add `audio_host` to the examples build.
- [ ] Push and verify GitHub Actions fails because the example implementation is not present yet.

### Task 2: Implement the Pico-PIO-USB Audio Host example

**Files:**
- Create: `examples/audio_host/main.c`
- Create: `examples/audio_host/tusb_config.h`
- Create: `examples/audio_host/bsp/board_api.h`
- Modify: `examples/audio_host/CMakeLists.txt`

- [ ] Initialize the Pico clock and stdio.
- [ ] Configure TinyUSB RHPort 1 with `TUH_CFGID_RPI_PIO_USB_CONFIGURATION` before host init.
- [ ] Compile TinyUSB's upstream `audio_app.c` directly from `${PICO_TINYUSB_PATH}`.
- [ ] Enable `CFG_TUH_AUDIO` for UAC1/UAC2 and 256-byte capture/playback buffers.
- [ ] Compile Pico-PIO-USB with `PIO_USB_EP_SIZE=256` for this target only.
- [ ] Build using the PIO USB TinyUSB HCD.

### Task 3: CI and regression verification

**Files:**
- Modify: `.github/workflows/build.yml` only if artifact coverage needs adjustment.

- [ ] Verify Actions builds the audio target for supported Pico SDK/board matrix entries.
- [ ] Verify existing `usb_device` and `host_hid_to_device_cdc` targets still build.
- [ ] Inspect logs for warnings/errors specific to isochronous packet sizing or TinyUSB Audio Host integration.
- [ ] Review the aggregate diff before completion.
