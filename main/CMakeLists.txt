idf_component_register(SRCS
    "abort.cpp"
    "als.cpp"
    "display.cpp"
    "filesystem.cpp"
    "main.cpp"
    "ntp.cpp"
    "rtc.cpp"
    "tz_info_local.cpp"
    "unique_id.cpp"
    "wifi_control.cpp"
    "wpa_key_gen.cpp"
    "nvs_local.cpp"
    "http/server_control.cpp"
    "http/api_handlers.cpp"
    "http/file_server.cpp"
INCLUDE_DIRS "."
)

add_custom_command(OUTPUT ${BUILD_DIR}/spiffs-image
     COMMAND ${PROJECT_DIR}/prepare_spiffs_image.sh ${PROJECT_DIR} ${BUILD_DIR}
     VERBATIM)

add_custom_target(spiffs-image DEPENDS ${BUILD_DIR}/spiffs-image)
add_dependencies(app spiffs-image)

spiffs_create_partition_image(storage ${BUILD_DIR}/spiffs-image FLASH_IN_PROJECT)