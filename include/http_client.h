#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_
#include <WiFi.h>
#include <HTTPClient.h>
#include "public.h"
#include "trace.h"

extern std::vector<AIDA64_DATA> aida64DataList;

void taskHttpClient(void *param);
void initHttpClient();
#endif