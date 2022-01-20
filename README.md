NTP synchronized desktop 7-segment clock
=======================================================
Features
--------
1. NTP sunchronization using WiFi.
2. RTC backup - will show time from a high precision RTC if WiFi is not available.
3. Automatic display brightness control.
4. Automatic timezone data update from online TZDATA database.
5. Settings via WiFi AP mode and a web page.

Pictures
--------
![Front view](/readme_assets/assembly_front.jpg)
![Inside view](/readme_assets/assembly_guts.jpg)

Components
----------
1. ESP32 controller with embedded WiFi.
2. Adafruit 1.2" 7-segment display.
3. DS3231 RTC module.
4. Adafruit VEML7700 lux sensor module.
5. Custom I2C and power hub board.
6. Several jumper wires.
7. M3, M2.5 and M2 bolts of variuos lengths - unfortunately, modules have different screw hole sizes.
8. 3D printed plastic enclosure.

Software
--------
1. ESP-IDF (built on FreeRTOS).
2. Arduino as ESP-IDF component.
3. Formantic UI.
