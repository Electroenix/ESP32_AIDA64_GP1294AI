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

    //wifi
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.printf("[WIFI] Connecting to %s...\r\n", WIFI_SSID);
        delay(1000);
    }
    Serial.printf("[WIFI] Connect Succeed!\r\n");

    //Display
    u8g2.begin();
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setFontPosTop();
    Serial.printf("[VFD DISPLAY] Init finish\r\n");
    
    Serial.print("[SYSTEM] READY!\r\n");

    // thread
    xTaskCreate(taskHttpClient, "taskHttpClient", 102400, NULL, 2, NULL);
}

void loop()
{
    // put your main code here, to run repeatedly:
}

