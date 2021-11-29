#include "wifi_local.h"

#define CONNECT_DELAY_MS 4096
#define CONNECT_ATTEMPTS 16

int status = WL_IDLE_STATUS;

void printMacAddress(byte mac[])
{
    for (int i = 5; i >= 0; i--)
    {
        if (mac[i] < 16)
        {
            Serial.print("0");
        }
        Serial.print(mac[i], HEX);

        if (i > 0)
        {
            Serial.print(":");
        }
    }
    Serial.println();
}

void printWifiData()
{
    WiFi.printDiag(Serial);
}

bool wifiConnect()
{
    Serial.println("Connecting WiFi.");
    int connect_attempts = 0;
    while (status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(WIFI_SSID);
        status = WiFi.begin(WIFI_SSID, WIFI_PASS);

        connect_attempts++;
        if (connect_attempts > CONNECT_ATTEMPTS)
        {
            Serial.println("WiFi connection timeout.");
            wifiEnd();
            return false;
        }
        delay(CONNECT_DELAY_MS);
    }

    Serial.println("WiFi connected.");
    return true;
}

bool wifiBegin()
{
    Serial.println("Starting WiFi module.");

    if (wifiConnect())
    {
        printWifiData();
        return true;
    }
    else
    {
        return false;
    }
}

void wifiEnd()
{
    Serial.println("Disconnecting WiFi.");
    WiFi.disconnect();
}

void wifiDisconnect()
{
    Serial.println("Disconnecting WiFi.");
    WiFi.disconnect();
}

bool wifiReconnect()
{
    Serial.println("Trying to reconnect WiFi.");
    wifiDisconnect();
    return wifiConnect();
}