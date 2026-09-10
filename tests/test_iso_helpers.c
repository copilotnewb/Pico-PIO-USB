#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define PIO_USB_EP_SIZE 256
#include "pio_usb_iso.h"

_Static_assert(PIO_USB_EP_SIZE == 256,
               "PIO_USB_EP_SIZE compile-time override must be preserved");

int main(void) {
  assert(pio_usb_ep_is_periodic(EP_ATTR_INTERRUPT));
  assert(pio_usb_ep_is_periodic(EP_ATTR_ISOCHRONOUS));
  assert(!pio_usb_ep_is_periodic(EP_ATTR_BULK));

  assert(pio_usb_ep_interval_frames(EP_ATTR_INTERRUPT, 7) == 7);
  assert(pio_usb_ep_interval_frames(EP_ATTR_ISOCHRONOUS, 1) == 1);
  assert(pio_usb_ep_interval_frames(EP_ATTR_ISOCHRONOUS, 2) == 2);
  assert(pio_usb_ep_interval_frames(EP_ATTR_ISOCHRONOUS, 4) == 8);
  assert(pio_usb_ep_interval_frames(EP_ATTR_ISOCHRONOUS, 16) == 32768);

  assert(!pio_usb_ep_uses_handshake(EP_ATTR_ISOCHRONOUS));
  assert(pio_usb_ep_uses_handshake(EP_ATTR_INTERRUPT));
  assert(!pio_usb_ep_retries(EP_ATTR_ISOCHRONOUS));
  assert(pio_usb_ep_retries(EP_ATTR_BULK));

  return 0;
}
