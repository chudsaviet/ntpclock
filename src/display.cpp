#include "display.h"

#define DISPLAY_ADDRESS 0x70

Adafruit_7segment clockDisplay = Adafruit_7segment();

void displayBegin()
{
    clockDisplay.begin(DISPLAY_ADDRESS);
}

void display(time_t time, bool showColons, uint8_t brightness)
{
    clockDisplay.setBrightness(brightness);
    clockDisplay.print(hour(time) * 100 + minute(time), DEC);
    if (showColons)
    {
        clockDisplay.drawColon(second(time) % 2 == 0);
    }
    else
    {
        clockDisplay.drawColon(false);
    }
    clockDisplay.writeDisplay();
}

void displayUpdate(uint8_t currentBrightness, bool showColons)
{
    timeval currentTime;
    gettimeofday(&currentTime, NULL);

    currentTime.tv_sec = utc2local(currentTime.tv_sec);

    display(currentTime.tv_sec, showColons, currentBrightness);
}