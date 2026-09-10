# Pico-PIO-USB

USB host/device implementation using PIO of raspberry pi pico (RP2040).

You can add additional USB port to RP2040.

## Demo

https://user-images.githubusercontent.com/43873124/146642806-bdf34af6-4342-4a95-bfca-229cdc4bdca2.mp4

## Project status

|Planned Features|Status|
|-|-|
|FS Host|✔|
|FS Isochronous Host|✔|
|LS Host|✔|
|Hub support|✔|
|Multi port|✔|
|FS Device|✔|

## Examples

- [host_hid_to_device_cdc.c](examples/host_hid_to_device_cdc/host_hid_to_device_cdc.c) which print mouse/keyboard report from host port to device port's cdc. TinyUSB is used to manage both device (native usb) and host (pio usb) stack.
- [usb_device.c](examples/usb_device/usb_device.c) is a HID USB FS device sample which moves mouse cursor every 0.5s. External 1.5kohm pull-up register is necessary to D+ pin (Default is gp0).

```bash
cd examples
mkdir build
cd build
cmake ..
make
# Copy UF2 file in capture_hid_report/ or usbdevice/ to RPiPico
```

### Isochronous host endpoints

The default maximum endpoint packet size remains 64 bytes to preserve the existing RAM footprint. Applications that need larger full-speed isochronous packets can override `PIO_USB_EP_SIZE` at compile time. For example, 256 bytes is sufficient for 48 kHz, 16-bit stereo audio packets:

```bash
cmake -DCMAKE_C_FLAGS="-DPIO_USB_EP_SIZE=256" ..
```

Endpoint descriptors whose maximum packet size exceeds the configured `PIO_USB_EP_SIZE` are rejected when opened. The supported compile-time range is 64 to 1023 bytes.

Another sample program for split keyboard with QMK

[https://github.com/sekigon-gonnoc/qmk_firmware/tree/rp2040/keyboards/pico_pico_usb](https://github.com/sekigon-gonnoc/qmk_firmware/tree/rp2040/keyboards/pico_pico_usb)

## Resource Usage


- 1 PIO, 3 state machines, 32 instructions
- Two GPIO for D+/D- (Series 22ohm resitors are better)
- 15KB ROM and RAM
- (For Host) One 1ms repeating timer
- (For Device) One PIO IRQ for receiver
