set(target_name audio_host)
set(tinyusb_audio_host_dir ${PICO_TINYUSB_PATH}/examples/host/audio_host/src)

add_executable(${target_name})

target_sources(${target_name} PRIVATE
  main.c
  ${tinyusb_audio_host_dir}/audio_app.c
  ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

target_compile_definitions(${target_name} PRIVATE
  PIO_USB_USE_TINYUSB
  # The KTMicro/UGREEN test adapter advertises wMaxPacketSize=384 on its
  # 16-bit playback alternate, even though 44.1/48 kHz transfers are smaller.
  PIO_USB_EP_SIZE=384
  # Standard DeskHop routes its USB-A host connector to GP14 (D+) / GP15 (D-).
  PIO_USB_DP_PIN_DEFAULT=14
)

target_include_directories(${target_name} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${tinyusb_audio_host_dir}
)

target_link_options(${target_name} PRIVATE -Xlinker --print-memory-usage)
target_compile_options(${target_name} PRIVATE -Wall -Wextra)
target_link_libraries(${target_name} PRIVATE pico_stdlib pico_pio_usb tinyusb_host)

pico_add_extra_outputs(${target_name})
