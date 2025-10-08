#ifndef _TRACE_H_
#define _TRACE_H_

#include <Arduino.h>
extern "C" {
  #include "esp_log.h"
  #include "esp_system.h"
}


extern const char* MAIN_TAG;
extern const char* WIFI_TAG;
extern const char* HTTP_TAG;
extern const char* DISPLAY_TAG;
extern const char* WEB_SERVER_TAG;
extern const char* FILE_SYSTEM_TAG;

extern void initLogger(void);

#endif