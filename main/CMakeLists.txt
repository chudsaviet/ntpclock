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
    "webserver_control.cpp"
INCLUDE_DIRS "."
)

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)