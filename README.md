NTP synchronized desktop 7-segment clock
=======================================================
Features
--------
1. *NTP* sunchronization using WiFi.
2. *RTC* backup - will show time from a high precision RTC if WiFi is not available.
3. Automatic display *brightness* control.
4. Automatic *timezone* data update from online TZDATA database.
5. Settings via WiFi AP mode and a web page.

Pictures
--------
![Front view](/readme_assets/assembly_front.jpg)
![Back view](/readme_assets/assembly_back.jpg)
![Inside view](/readme_assets/assembly_guts.jpg)

Bill of materials
-----------------

| Part                            | Price |
| ------------------------------- | ------|
| ESP32, Generic DevKit V1        | $8    |
| Adafruit 1.2" 7-segment display | $18   |
| DS3231*SN* RTC module           | $18   |
| Adafruit VEML7700 lux sensor    | $5    |
| Custom I2C and power hub board  | $5    |
| Jumper wires, Female-Female     | $2    |
| M3, M2.5 and M2 bolts           | $5    |
| 3D printed plastic enclosure    | $8    |
| Tint film (ex. for cars)        | $1    |
| ------------------------------- | ----- |
| Total                           | $70   |

Building and running the device
-------------------------------
1. Manufacture I2C/Power hub using files from `cad/i2c_header_x5`.
2. 3D print enclosure using files from `cad/enclosure`.
3. Apply tint film to the display module.
4. *Assemble the device.*
5. Install ESP-IDF *v4.4* - https://github.com/espressif/esp-idf/tree/release/v4.4 .
6. Clone repository and *init submodules recursively*.
7. Copy `main/secrets.h` from `main/secrets.h.example` and set your own setting inside.
8. Using instructions from EPS-IDF, build and flash the firmware.

Changing settings after flashing firmware
-----------------------------------------
1. To change settings after flash, connect the device to power and press 'SET' button on the bottom.
2. Device will create a WiFi AP with SSID looking like `ntpclock-XXXXXXXX`.
3. The password will be shown scrolling on display.
4. Connect to AP and open http://192.168.4.1 .

![UI wifi tab](/readme_assets/ui_wifi.png)
![UI time tab](/readme_assets/ui_time.png)
