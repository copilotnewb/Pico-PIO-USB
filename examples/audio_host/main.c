/*
 * Pico-PIO-USB TinyUSB Audio Host integration example.
 *
 * Uses TinyUSB's upstream audio_host application logic while running the host
 * controller on RP2040/RP2350 PIO USB (rhport 1).
 */

#include <stdio.h>

#include "hardware/clocks.h"
#include "pico/stdlib.h"

#include "pio_usb.h"
#include "tusb.h"
#include "app.h"

int main(void) {
  // PIO USB requires a system clock that is an integer multiple of 12 MHz.
  set_sys_clock_khz(120000, true);
  stdio_init_all();
  sleep_ms(10);

  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

  // Run TinyUSB host on the PIO USB controller (rhport 1).
  tuh_init(1);

  printf("Pico-PIO-USB TinyUSB Audio Host example\r\n");
  printf("Connect a UAC1/UAC2 USB audio device to the PIO USB port\r\n");

  while (true) {
    tuh_task();
    audio_app_task();
    defer_queue_task();
    led_blinking_task();
  }

  return 0;
}
