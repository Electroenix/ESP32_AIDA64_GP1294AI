#include "trace.h"

const char* MAIN_TAG = "MAIN";
const char* WIFI_TAG = "WIFI";
const char* HTTP_TAG = "HTTP";
const char* DISPLAY_TAG = "DISPLAY";
const char* WEB_SERVER_TAG = "WEB_SERVER";
const char* FILE_SYSTEM_TAG = "FILE_SYSTEM";

void initLogger() {
    Serial.begin(115200);
}
