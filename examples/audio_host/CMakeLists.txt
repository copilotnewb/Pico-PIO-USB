set(target_name audio_host)
set(tinyusb_audio_host_dir ${PICO_TINYUSB_PATH}/examples/host/audio_host/src)

add_executable(${target_name})

target_sources(${target_name} PRIVATE
  main.c
  diagnostics.c
  ${tinyusb_audio_host_dir}/audio_app.c
  ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

# Keep the upstream application intact. Observing callbacks forward to these
# renamed definitions exactly once. Renaming is scoped to this source file.
set_source_files_properties(${tinyusb_audio_host_dir}/audio_app.c PROPERTIES
  COMPILE_DEFINITIONS
  "tuh_audio_mount_cb=audio_example_mount_cb;tuh_audio_umount_cb=audio_example_umount_cb;tuh_audio_capture_cb=audio_example_capture_cb;tuh_audio_playback_cb=audio_example_playback_cb;tuh_audio_event_cb=audio_example_event_cb"
)

# CI follows TinyUSB master. Record the actual checkout in every diagnostic log.
execute_process(COMMAND git rev-parse --short=12 HEAD
  WORKING_DIRECTORY ${PICO_TINYUSB_PATH}
  OUTPUT_VARIABLE audio_host_tinyusb_rev OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET RESULT_VARIABLE audio_host_git_result)
if(NOT audio_host_git_result EQUAL 0)
  set(audio_host_tinyusb_rev unknown)
endif()

target_compile_definitions(${target_name} PRIVATE
  PIO_USB_USE_TINYUSB
  # KTMicro advertises 384 even when the selected rate needs smaller packets.
  PIO_USB_EP_SIZE=384
  # DeskHop USB-A: GP14 D+, GP15 D-.
  PIO_USB_DP_PIN_DEFAULT=14
  # Fast UART reduces main-loop stalls from diagnostics. No per-ISO-packet log.
  PICO_DEFAULT_UART_BAUD_RATE=115200
  AUDIO_HOST_TINYUSB_REV="${audio_host_tinyusb_rev}"
)

target_include_directories(${target_name} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${tinyusb_audio_host_dir}
)

target_link_options(${target_name} PRIVATE -Xlinker --print-memory-usage
  -Wl,--wrap=hcd_port_reset -Wl,--wrap=hcd_port_reset_end
  -Wl,--wrap=hcd_setup_send -Wl,--wrap=hcd_edpt_open -Wl,--wrap=hcd_edpt_xfer
)
target_compile_options(${target_name} PRIVATE -Wall -Wextra)
target_link_libraries(${target_name} PRIVATE pico_stdlib pico_pio_usb tinyusb_host)

# Native USB CDC is not part of this standalone host test.
pico_enable_stdio_usb(${target_name} 0)
pico_enable_stdio_uart(${target_name} 1)
pico_add_extra_outputs(${target_name})
