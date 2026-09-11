/* TinyUSB host configuration for the Pico-PIO-USB audio_host example. */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_TUSB_OS OPT_OS_PICO

#define CFG_TUH_ENABLED 1
#define CFG_TUH_RPI_PIO_USB 1

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

#define CFG_TUH_ENUMERATION_BUFSIZE 512
#define CFG_TUH_HUB 0
#define CFG_TUH_DEVICE_MAX 1

#define CFG_TUH_CDC 0
#define CFG_TUH_HID 0
#define CFG_TUH_MSC 0
#define CFG_TUH_VENDOR 0
#define CFG_TUH_AUDIO 1

#define CFG_TUH_AUDIO_PROTOCOLS (TUH_AUDIO_PROTOCOL_UAC1 | TUH_AUDIO_PROTOCOL_UAC2)
#define CFG_TUH_AUDIO_MAX 1
#define CFG_TUH_AUDIO_EPIN_BUFSIZE 256
#define CFG_TUH_AUDIO_EPOUT_BUFSIZE 256
#define CFG_TUH_AUDIO_STREAM_BUFSIZE 1024

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
