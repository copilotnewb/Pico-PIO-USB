/**
 * Copyright (c) 2021 sekigon-gonnoc
 *                    Ha Thach (thach@tinyusb.org)
 */

#pragma GCC push_options
#pragma GCC optimize("-O3")

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hardware/sync.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"

#include "pio_usb.h"
#include "pio_usb_iso.h"
#include "pio_usb_ll.h"
#include "usb_crc.h"

enum {
  TRANSACTION_MAX_RETRY = 3, // Number of times to retry a failed transaction
};

static alarm_pool_t *_alarm_pool = NULL;
static repeating_timer_t sof_rt;
// The sof_count may be incremented and then read on different cores.
static volatile uint32_t sof_count = 0;
static bool timer_active;

static volatile bool cancel_timer_flag;
static volatile bool start_timer_flag;
static __unused uint32_t int_stat;
static uint8_t sof_packet[4] = {USB_SYNC, USB_PID_SOF, 0x00, 0x10};
static uint8_t sof_packet_encoded[4 * 2 * 7 / 6 + 2];
static uint8_t sof_packet_encoded_len;
static uint8_t keepalive_encoded[1];

static bool sof_timer(repeating_timer_t *_rt);

//--------------------------------------------------------------------+
// Application API
//--------------------------------------------------------------------+

static void start_timer(alarm_pool_t *alarm_pool) {
  if (timer_active) {
    return;
  }

  if (alarm_pool != NULL) {
    alarm_pool_add_repeating_timer_us(alarm_pool, -1000, sof_timer, NULL,
                                      &sof_rt);
  }

  timer_active = true;
}

static __unused void stop_timer(void) {
  cancel_repeating_timer(&sof_rt);
  timer_active = false;
}

usb_device_t *pio_usb_host_init(const pio_usb_configuration_t *c) {
  pio_port_t *pp = PIO_USB_PIO_PORT(0);
  root_port_t *root = PIO_USB_ROOT_PORT(0);

  pio_usb_bus_init(pp, c, root);
  root->mode = PIO_USB_MODE_HOST;

  float const cpu_freq = (float)clock_get_hz(clk_sys);
  pio_calculate_clkdiv_from_float(cpu_freq / 48000000,
                                  &pp->clk_div_fs_tx.div_int,
                                  &pp->clk_div_fs_tx.div_frac);
  pio_calculate_clkdiv_from_float(cpu_freq / 6000000,
                                  &pp->clk_div_ls_tx.div_int,
                                  &pp->clk_div_ls_tx.div_frac);

  pio_calculate_clkdiv_from_float(cpu_freq / 96000000,
                                  &pp->clk_div_fs_rx.div_int,
                                  &pp->clk_div_fs_rx.div_frac);
  pio_calculate_clkdiv_from_float(cpu_freq / 12000000,
                                  &pp->clk_div_ls_rx.div_int,
                                  &pp->clk_div_ls_rx.div_frac);

  sof_packet_encoded_len =
      pio_usb_ll_encode_tx_data(sof_packet, sizeof(sof_packet), sof_packet_encoded);
  pio_usb_ll_encode_tx_data(NULL, 0, keepalive_encoded);

  if (!c->skip_alarm_pool) {
    _alarm_pool = c->alarm_pool;
    if (!_alarm_pool) {
      _alarm_pool = alarm_pool_create(2, 1);
    }
  }
  start_timer(_alarm_pool);

  return &pio_usb_device[0];
}

void pio_usb_host_stop(void) {
  cancel_timer_flag = true;
  while (cancel_timer_flag) {
    continue;
  }
}

void pio_usb_host_restart(void) {
  start_timer_flag = true;
  while (start_timer_flag) {
    continue;
  }
}

//--------------------------------------------------------------------+
// Bus functions
//--------------------------------------------------------------------+

static void __no_inline_not_in_flash_func(override_pio_program)(PIO pio, const pio_program_t* program, uint offset) {
    for (uint i = 0; i < program->length; ++i) {
      uint16_t instr = program->instructions[i];
      pio->instr_mem[offset + i] =
          pio_instr_bits_jmp != _pio_major_instr_bits(instr) ? instr
                                                             : instr + offset;
    }
}

static __always_inline void override_pio_rx_program(PIO pio,
                                             const pio_program_t *program,
                                             const pio_program_t *debug_program,
                                             uint offset, int debug_pin) {
  if (debug_pin < 0) {
    override_pio_program(pio, program, offset);
  } else {
    override_pio_program(pio, debug_program, offset);
  }
}

static void
__no_inline_not_in_flash_func(configure_tx_program)(pio_port_t *pp,
                                                    root_port_t *port) {
  if (port->pinout == PIO_USB_PINOUT_DPDM) {
    pp->fs_tx_program = &usb_tx_dpdm_program;
    pp->fs_tx_pre_program = &usb_tx_pre_dpdm_program;
    pp->ls_tx_program = &usb_tx_dmdp_program;
  } else {
    pp->fs_tx_program = &usb_tx_dmdp_program;
    pp->fs_tx_pre_program = &usb_tx_pre_dmdp_program;
    pp->ls_tx_program = &usb_tx_dpdm_program;
  }
}

static void __no_inline_not_in_flash_func(configure_fullspeed_host)(
    pio_port_t *pp, root_port_t *port) {
  pp->low_speed = false;
  configure_tx_program(pp, root_port_t *port);
}
