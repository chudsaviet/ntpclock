#include "display.h"

#define DISPLAY_ADDRESS 0x70

Adafruit_7segment clockDisplay = Adafruit_7segment();

void displayBegin()
{
    clockDisplay.begin(DISPLAY_ADDRESS);
}

void display(time_t time, bool colon)
{
    clockDisplay.setBrightness(16);
    clockDisplay.print(hour(time) * 100 + minute(time), DEC);
    clockDisplay.writeDisplay();

    clockDisplay.setBrightness(16);
    clockDisplay.drawColon(colon);
    clockDisplay.writeColon();
}