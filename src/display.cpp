#include "display.h"

#define DISPLAY_ADDRESS 0x70

Adafruit_7segment clockDisplay = Adafruit_7segment();

void displayBegin()
{
    clockDisplay.begin(DISPLAY_ADDRESS);
}

void display(time_t time)
{
    clockDisplay.setBrightness(10);
    clockDisplay.print(hour(time) * 100 + minute(time), DEC);
    clockDisplay.drawColon(second(time) % 2 == 0);
    clockDisplay.writeDisplay();
}