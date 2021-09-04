#include "wifi.h"

#define CONNECT_DELAY_MS 1024
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

void printCurrentNet()
{
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    byte bssid[6];

    WiFi.BSSID(bssid);
    Serial.print("BSSID: ");
    printMacAddress(bssid);

    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.println(rssi);

    byte encryption = WiFi.encryptionType();
    Serial.print("Encryption Type:");
    Serial.println(encryption, HEX);
}

void printWifiData()
{
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    byte mac[6];
    WiFi.macAddress(mac);
    Serial.print("MAC address: ");
    printMacAddress(mac);
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

    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("Communication with WiFi module failed!");
        return false;
    }

    String fv = WiFi.firmwareVersion();

    if (fv < WIFI_FIRMWARE_LATEST_VERSION)
    {
        Serial.println("Please upgrade the firmware!");
    }

    if (wifiConnect())
    {
        printCurrentNet();
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
    WiFi.end();
}

void wifiLowPower()
{
    Serial.println("Putting WiFi to low power mode.");
    WiFi.lowPowerMode();
}

void wifiNoLowPower()
{
    Serial.println("Putting WiFi to full operational mode.");
    WiFi.noLowPowerMode();
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