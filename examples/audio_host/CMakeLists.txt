set(target_name audio_host)
add_executable(${target_name})

target_sources(${target_name} PRIVATE
  main.c
)

target_compile_definitions(${target_name} PRIVATE
  PIO_USB_USE_TINYUSB
  PIO_USB_EP_SIZE=256
)

target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(${target_name} PRIVATE pico_stdlib pico_pio_usb tinyusb_host)
pico_add_extra_outputs(${target_name})
