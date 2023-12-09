#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_
#include <WiFi.h>
#include <HTTPClient.h>
#include "public.h"

#define httpPrintLog(format, arg...) UARTPrintf("[HTTP] " format, ##arg)

extern std::vector<AIDA64_DATA> aida64DataList;

extern void taskHttpClient(void *param);
extern void parseAida64HTML(char *htmlData, std::vector<AIDA64_DATA> &dataList);
extern void parseAida64Data(char *src, std::vector<AIDA64_DATA> &dataList);
extern void strremove(char* src, char remove);
#endif