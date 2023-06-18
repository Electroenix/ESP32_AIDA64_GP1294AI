#include "http_client.h"
#include <lwip/sockets.h>
#include "config.h"
#include "display.h"

char httpDataBuffer[4096];
std::vector<AIDA64_DATA> aida64DataList;

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
        httpClient.begin(HTTP_HOST, HTTP_PORT);
        httpCode = httpClient.GET();

        // httpCode will be negative on error
        if (httpCode == HTTP_CODE_OK)
        {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET HTML succeed: %d\n", httpCode);
            String getHTML = httpClient.getString().c_str();

            //Parse HTML
            parseAida64HTML(const_cast<char*>(getHTML.c_str()), aida64DataList);
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
                recv_len = recv(fd, httpDataBuffer, 1024, 0);

                if(recv_len <= 0)
                {
                    Serial.printf("[HTTP] Connect error!\n");
                    goto END;
                }

                httpDataBuffer[recv_len] = '\0';
                //Serial.printf("%s\r\n", payload);

                //parse data
                parseAida64Data(httpDataBuffer, aida64DataList);
                //display data
                display.displayAida64Data(aida64DataList);
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

    memset(httpDataBuffer, 0, sizeof(httpDataBuffer));
    snprintf(httpDataBuffer, temp2 - temp1, temp1);
    Serial.printf("%s\r\n", httpDataBuffer);

    temp1 = httpDataBuffer;
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

        aida64DataList.push_back(data);

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