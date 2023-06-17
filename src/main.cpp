#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <vector>
#include <lwip/sockets.h>
#include "U8g2lib.h"

/* 
 * GP1294AI PIN define
 * use 3 wire SPI
 * SCL -> 18
 * SDA -> 23
 */
#define CS 5
#define RST 21

typedef struct
{
    char user[32];
    char pass[32];
} WIFI_INFO;

typedef struct
{
    char host[128];
    int port;
    char dataBuffer[4096];
} HTTPCLIENT_CONTEXT;

typedef struct
{
    char id[32];
    char val[32];
}AIDA64_DATA;


WIFI_INFO g_wifi_info = {"USER", "PASS"};
HTTPCLIENT_CONTEXT g_httpclient_context = {"HOST", 8080};
std::vector<AIDA64_DATA> adia64DataList; //save received aida64 data

U8G2_GP1294AI_256X48_F_3W_HW_SPI u8g2(U8G2_R3, /* cs=*/ CS, /* reset=*/ RST);

extern void taskHttpClient(void *param);
extern void parseAida64HTML(char *htmlData, std::vector<AIDA64_DATA> &dataList);
extern void parseAida64Data(char *src, std::vector<AIDA64_DATA> &dataList);
extern void displayAida64Data(std::vector<AIDA64_DATA> &dataList);
extern void strremove(char* src, char remove);

void setup()
{
    // put your setup code here, to run once:
    //Serial
    Serial.begin(115200);
    Serial.print("[SYSTEM] Initial start...\r\n");

    //wifi
    WiFi.begin(g_wifi_info.user, g_wifi_info.pass);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.printf("[WIFI] Connecting to %s...\r\n", g_wifi_info.user);
        delay(1000);
    }
    Serial.printf("[WIFI] Connect Succeed!\r\n", g_wifi_info.user);

    //VFD
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

void taskHttpClient(void *param)
{
    HTTPClient httpClient;
    WiFiClient *tcpStream = NULL;
    int httpCode = 0;
    int fd = 0;//tcp socket fd
    int recv_len = 0;

    Serial.print("[TASK] taskHttpClient run!\r\n");

    while(1)
    {
        /*
         * Request HTML
         * 解析HTML获得需要显示的项目列表
         */
        httpClient.begin(g_httpclient_context.host, g_httpclient_context.port);
        httpCode = httpClient.GET();

        // httpCode will be negative on error
        if (httpCode == HTTP_CODE_OK)
        {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET HTML succeed: %d\n", httpCode);
            String getHTML = httpClient.getString().c_str();

            //Parse HTML
            parseAida64HTML(const_cast<char*>(getHTML.c_str()), adia64DataList);
        }
        else
        {
            Serial.printf("[HTTP] GET HTML failed, error: %s\n", httpClient.errorToString(httpCode).c_str());
            goto END;
        }

        /*
         * Request update data
         * 发送GET /sse 请求后，AIDA64会不断回复刷新的数据
         */

        httpClient.setURL("/sse");
        httpCode = httpClient.GET();

        if(httpCode == HTTP_CODE_OK)
        {
            while (1)
            {
                tcpStream = httpClient.getStreamPtr();
                fd = tcpStream->fd();
                recv_len = recv(fd, g_httpclient_context.dataBuffer, 1024, 0);

                if(recv_len <= 0)
                {
                    Serial.printf("[HTTP] Connect error!\n");
                    goto END;
                }

                g_httpclient_context.dataBuffer[recv_len] = '\0';
                //Serial.printf("%s\r\n", payload);

                //parse data
                parseAida64Data(g_httpclient_context.dataBuffer, adia64DataList);
                //display data
                displayAida64Data(adia64DataList);
                //adia64DataList.clear();
            }
        }
        else
        {
            Serial.printf("[HTTP] GET /sse failed, error: %s\n", httpClient.errorToString(httpCode).c_str());
            goto END;
        }

    END:
        httpClient.end();
    }
}

void parseAida64HTML(char *htmlData, std::vector<AIDA64_DATA> &dataList)
{
    char *temp1 = NULL;
    char *temp2 = NULL;
    AIDA64_DATA data = {0};

    /*
     * 接收到的HTML有如下结构
     * ...
     * <body onload="MyOnLoad()">
     * <div id="page0">
     * <span id="xxx1" ...>XXX</span>
     * <span id="xxx2" ...>XXX</span>
     * ...
     * <span id="xxxn" ...>XXX</span>
     * </div>
     * </body>
     * ...
     * 其中span标签的内容即是在AIDA64中设置的LCD项目，需要将id和内容提取出来，保存在adia64DataList中
     * 之后会发送请求获取刷新数据，通过对比id，修改adia64DataList中对应的值
     */

    temp1 = strstr(htmlData, "<body");
    temp2 = strstr(htmlData, "</body>");

    memset(g_httpclient_context.dataBuffer, 0, sizeof(g_httpclient_context.dataBuffer));
    snprintf(g_httpclient_context.dataBuffer, temp2 - temp1, temp1);
    Serial.printf("%s\r\n", g_httpclient_context.dataBuffer);

    temp1 = g_httpclient_context.dataBuffer;
    do
    {
        temp1 = strstr(temp1, "<span");
        if(temp1 == NULL)
            return;
        
        //get id
        temp1 = strstr(temp1, "id=\"");
        if(temp1 == NULL)
            return;
        temp1 += strlen("id=\"");

        temp2 = strstr(temp1, "\" ");
        if(temp2 == NULL)
            return;
        
        memset(data.id, 0, sizeof(data.id));
        strncpy(data.id, temp1, temp2 - temp1);

        Serial.printf("id: %s\r\n", data.id);

        //get val
        temp1 = strstr(temp1, ">");
        if(temp1 == NULL)
            return;
        temp1++;

        temp2 = strstr(temp1, "</span>");
        if(temp2 == NULL)
            return;
        memset(data.val, 0, sizeof(data.val));
        strncpy(data.val, temp1, temp2 - temp1);

        Serial.printf("val: %s\r\n", data.val);

        adia64DataList.push_back(data);

        temp1 = temp2 + strlen("</span>");
    } while (1);
}

void parseAida64Data(char *src, std::vector<AIDA64_DATA> &dataList)
{        
    char *temp1 = src;
    char *temp2 = NULL;
    AIDA64_DATA data = {0};

    /* 
     * ADIA64会回复以下格式的响应体:
     * data: Page0|{|}Simple1|数据1{|}Simple2|数据2{|}...
     * 需要将数据从字符串中提取出来
     */

    do
    {
        temp1 = strstr(temp1, "{|}");
        if(temp1 == NULL)
            return;
        temp1 += strlen("{|}");

        temp2 = strstr(temp1, "|");
        if(temp2 == NULL)
            return;
        
        
        memset(data.id, 0, sizeof(data.id));
        strncpy(data.id, temp1, temp2 - temp1);

        temp1 = temp2 + 1;

        temp2 = strstr(temp1, "{|}");
        if(temp2 == NULL)
            return;
        memset(data.val, 0, sizeof(data.val));
        strncpy(data.val, temp1, temp2 - temp1);

        for(int i = 0; i < dataList.size(); i++)
        {
            if(!strcmp(dataList[i].id, data.id))
            {
                strremove(data.val, ' ');
                strcpy(dataList[i].val, data.val);
                break;
            }
        }

        temp1 = temp2;
    } while (1);
}
void displayAida64Data(std::vector<AIDA64_DATA> &dataList)
{
    u8g2.clearBuffer();

    for(int i = 0; i < dataList.size(); i++)
    {
        Serial.printf("%s\r\n", dataList[i].val);
        u8g2.drawUTF8(0, i * 8, dataList[i].val);
    }
    
    u8g2.sendBuffer();
}

void strremove(char* src, char remove)
{
    char* c = src;

    do
    {
        if(*c == remove)
        {
            c++;
        }

        *src = *c;
        src++;
        c++;
    } while (*c != '\0');
    
    *src = '\0';
}
