#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "usb_definitions.h"

// USB endpoint transfer type is stored in bmAttributes bits 0..1.
static inline uint8_t pio_usb_ep_transfer_type(uint8_t attr) {
  return attr & 0x03u;
}

static inline bool pio_usb_ep_is_isochronous(uint8_t attr) {
  return pio_usb_ep_transfer_type(attr) == EP_ATTR_ISOCHRONOUS;
}

static inline bool pio_usb_ep_is_periodic(uint8_t attr) {
  uint8_t const type = pio_usb_ep_transfer_type(attr);
  return type == EP_ATTR_INTERRUPT || type == EP_ATTR_ISOCHRONOUS;
}

// Full-speed interrupt endpoints encode bInterval directly in frames. Full-speed
// isochronous endpoints encode it as 2^(bInterval-1) frames (USB 2.0 9.6.6).
static inline uint16_t pio_usb_ep_interval_frames(uint8_t attr,
                                                  uint8_t b_interval) {
  if (pio_usb_ep_is_isochronous(attr)) {
    // Isochronous bInterval is defined for 1..16. Clamp invalid descriptors so
    // they cannot create an undefined shift or a permanently starved endpoint.
    if (b_interval == 0) {
      return 1;
    }
    if (b_interval > 16) {
      b_interval = 16;
    }
    return (uint16_t)(1u << (b_interval - 1u));
  }

  return b_interval ? b_interval : 1;
}

static inline bool pio_usb_ep_uses_handshake(uint8_t attr) {
  return !pio_usb_ep_is_isochronous(attr);
}

static inline bool pio_usb_ep_retries(uint8_t attr) {
  return !pio_usb_ep_is_isochronous(attr);
}
