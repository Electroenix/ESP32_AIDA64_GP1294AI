#ifndef _TRACE_H_
#define _TRACE_H_

extern "C" {
  #include "esp_log.h"
  #include <esp32-hal-log.h>
}

#undef CONFIG_LOG_MAXIMUM_LEVEL
#define CONFIG_LOG_MAXIMUM_LEVEL CORE_DEBUG_LEVEL

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV

#define ESP_LOGE( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR,   tag, ARDUHAL_LOG_FORMAT(E, format) __VA_OPT__(,) __VA_ARGS__)
#define ESP_LOGW( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_WARN,    tag, ARDUHAL_LOG_FORMAT(W, format) __VA_OPT__(,) __VA_ARGS__)
#define ESP_LOGI( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_INFO,    tag, ARDUHAL_LOG_FORMAT(I, format) __VA_OPT__(,) __VA_ARGS__)
#define ESP_LOGD( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_DEBUG,   tag, ARDUHAL_LOG_FORMAT(D, format) __VA_OPT__(,) __VA_ARGS__)
#define ESP_LOGV( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_VERBOSE, tag, ARDUHAL_LOG_FORMAT(V, format) __VA_OPT__(,) __VA_ARGS__)

#undef ARDUHAL_LOG_FORMAT
#define ARDUHAL_LOG_FORMAT(letter, format)				\
  ARDUHAL_LOG_COLOR_##letter "[%s:%u] %s(): " format ARDUHAL_LOG_RESET_COLOR,  \
    pathToFileName(__FILE__), __LINE__, __FUNCTION__

#undef LOG_FORMAT
#define LOG_FORMAT(letter, format)  ARDUHAL_LOG_COLOR_ ## letter "[%6u][" #letter "][%s]" format ARDUHAL_LOG_RESET_COLOR "\n"

extern const char* MAIN_TAG;
extern const char* WIFI_TAG;
extern const char* HTTP_TAG;
extern const char* DISPLAY_TAG;
extern const char* WEB_SERVER_TAG;
extern const char* FILE_SYSTEM_TAG;

extern void initLogger(void);

#endif