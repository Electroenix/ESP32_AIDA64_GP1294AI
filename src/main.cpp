#include <Arduino.h>
#include "http_client.h"
#include "display.h"
#include "config.h"

void setup()
{
    // put your setup code here, to run once:
    //Serial
    Serial.begin(115200);
    Serial.print("[SYSTEM] Initial start...\r\n");

    //Display
    display.begin();
    Serial.printf("[VFD DISPLAY] Init finish\r\n");
    display.clear();
    display.print("Welcome!\r\n");
    display.print("\r\nInit...\r\n");

    //wifi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    display.print("\r\nWIFI begin\r\n");
    display.print("\r\nConnect to\n" WIFI_SSID "\r\n");

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.printf("[WIFI] Connecting to %s...\r\n", WIFI_SSID);
        delay(1000);
    }
    Serial.printf("[WIFI] Connect Succeed!\r\n");
    display.print("\r\nSucceed!\r\n");
    
    Serial.print("[SYSTEM] READY!\r\n");
    display.print("\r\nREADY!\r\n");

    // thread
    xTaskCreate(taskHttpClient, "taskHttpClient", 102400, NULL, 2, NULL);
}

void loop()
{
    // put your main code here, to run repeatedly:
}

