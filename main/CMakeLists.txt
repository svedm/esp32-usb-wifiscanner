idf_component_register(SRCS "usb_descriptors.c" "main.c" "wifi/wifi.c"
                    INCLUDE_DIRS ".")

idf_component_get_property(tusb_lib espressif__tinyusb COMPONENT_LIB)
idf_component_get_property(tusb_path espressif__tinyusb COMPONENT_DIR)
target_include_directories(${tusb_lib} PRIVATE ".")
target_sources(${tusb_lib} PUBLIC "${tusb_path}/src/portable/espressif/esp32sx/dcd_esp32sx.c")