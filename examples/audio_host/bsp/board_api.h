/* Minimal TinyUSB board API shim for the Pico SDK audio_host example. */

#ifndef PICO_PIO_USB_AUDIO_HOST_BOARD_API_H_
#define PICO_PIO_USB_AUDIO_HOST_BOARD_API_H_

#include <stdbool.h>

#include "pico/stdlib.h"

static inline void board_led_write(bool state) {
#ifdef PICO_DEFAULT_LED_PIN
  static bool initialized = false;
  if (!initialized) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    initialized = true;
  }
  gpio_put(PICO_DEFAULT_LED_PIN, state);
#else
  (void)state;
#endif
}

#endif /* PICO_PIO_USB_AUDIO_HOST_BOARD_API_H_ */
