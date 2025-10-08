#include <LittleFS.h>
#include "trace.h"

#define FORMAT_LITTLEFS_IF_FAILED true

void initFileSystem()
{
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    ESP_LOGI(FILE_SYSTEM_TAG, "LittleFS Mount Failed");
    return;
  }
}