#include "arduino_task.h"

void vArduinoTask(void *pvParameters)
{
    while (true)
    {
        arduinoLoop();
    }
}