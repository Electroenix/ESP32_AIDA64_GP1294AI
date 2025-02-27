#include <Arduino.h>
#include "http_client.h"
#include "display.h"
#include "config.h"
#include "wifi_client.h"

/* default config */
int screen_dir = SCREEN_DIR_HORIZONTAL;
int loop_times = 0;

void setup()
{
    // put your setup code here, to run once:
    //Serial
    Serial.begin(115200);
    UARTPrintf("[SYSTEM] Initial start...\r\n");

    //Display
    display.begin(screen_dir);
    UARTPrintf("[VFD DISPLAY] Init finish\r\n");
    display.clear();
    display.print("Welcome!\r\n");

    // thread
    xTaskCreate(taskWifiClient, "taskWifiClient", 4096, NULL, 2, NULL);
    xTaskCreate(taskHttpClient, "taskHttpClient", 102400, NULL, 2, NULL);
}

void loop()
{
    // put your main code here, to run repeatedly:
#if 0
    char bufffer[32] = { 0 };
    sprintf(bufffer, "hello world! %d\r\n", loop_times++);
    display.print(bufffer);
    delay(1000);
#endif
}

