#include "http_client.h"
#include <lwip/sockets.h>
#include "config.h"
#include "display.h"
#include <regex>

char httpDataBuffer[4096];
std::vector<AIDA64_DATA> aida64DataList;

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

void parseAida64Data(const char *src, std::vector<AIDA64_DATA> &dataList)
{
    /* 
     * ADIA64会回复以下格式的响应体:
     * data: Page0|{|}Simple1|数据1{|}Simple2|数据2{|}...
     * 需要将数据从字符串中提取出来
     */
    
    AIDA64_DATA data = {0};
    std::string input(src);

    ESP_LOGI(HTTP_TAG, "recv aida64data, len: %d", strlen(src));
    ESP_LOGD(HTTP_TAG, "src:\r\n%s", src);

    // 定义正则表达式
    std::regex pattern("\\{\\|\\}(.*?)\\|(.*?)(?=\\{\\|\\})");

    // 搜索匹配项
    std::sregex_iterator it(input.begin(), input.end(), pattern);
    std::sregex_iterator end;
    
    while (it != end) {
        // 提取匹配的子串
        std::smatch match = *it;
        ESP_LOGD(HTTP_TAG, "match: %s, %s\r\n", match[1].str().c_str(), match[2].str().c_str());

        // 更新aida64DataList数据
        strcpy(data.id, match[1].str().c_str());
        strcpy(data.val, match[2].str().c_str());

        for(int i = 0; i < dataList.size(); i++)
        {
            if(!strcmp(dataList[i].id, data.id))
            {
                strremove(data.val, ' ');
                strcpy(dataList[i].val, data.val);
                break;
            }
        }

        ++it;
    }

    return;
}

void parseAida64HTML(const char *htmlData, std::vector<AIDA64_DATA> &dataList)
{
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
    
    AIDA64_DATA data = {0};

    std::string input(htmlData);

    aida64DataList.clear();
    ESP_LOGD(HTTP_TAG, "htmlData:\r\n%s", htmlData);

    // 定义正则表达式
    std::regex pattern("<span id=\"(.*?)\".*?>(.*?)<\\/span>");

    // 搜索匹配项
    std::sregex_iterator it(input.begin(), input.end(), pattern);
    std::sregex_iterator end;
    
    while (it != end) {
        // 提取匹配的子串
        std::smatch match = *it;
        ESP_LOGD(HTTP_TAG, "match: %s, %s", match[1].str().c_str(), match[2].str().c_str());

        // 加入aida64DataList
        strcpy(data.id, match[1].str().c_str());
        strcpy(data.val, match[2].str().c_str());
        aida64DataList.push_back(data);

        ++it;
    }

    return;
}

void taskHttpClient(void *param)
{
    int httpCode = 0;
    int fd = 0;//tcp socket fd
    int recv_len = 0;

    ESP_LOGI(HTTP_TAG, "taskHttpClient run!");

    while(1)
    {
        HTTPClient httpClient;
        WiFiClient *tcpStream = NULL;

        while(WiFi.status() != WL_CONNECTED)
        {
            delay(1000);
        }
        /*
         * Request HTML
         * 解析HTML获得需要显示的项目列表
         */
        httpClient.begin(HTTP_HOST, HTTP_PORT);
        httpClient.setReuse(false);
        httpCode = httpClient.GET();

        display.print("HTTP begin\r\n");
        display.print("GET / ...\r\n");

        // httpCode will be negative on error
        if (httpCode == HTTP_CODE_OK)
        {
            // HTTP header has been send and Server response header has been handled
            ESP_LOGI(HTTP_TAG, "GET HTML succeed: %d", httpCode);
            display.print("Succeed!\r\n");

            char *data_p = strstr(httpClient.getString().c_str(), "<body onload=\"MyOnLoad()\">");
            if(data_p == NULL)
            {
                ESP_LOGE(HTTP_TAG, "invalid html data, len: %d", strlen(httpClient.getString().c_str()));
                goto END;
            }

            //Parse HTML
            parseAida64HTML(data_p, aida64DataList);
        }
        else
        {
            ESP_LOGW(HTTP_TAG, "GET HTML failed, error: %s", httpClient.errorToString(httpCode).c_str());
            display.print("Failed!\r\n");
            goto END;
        }

        /*
         * Request update data
         * 发送GET /sse 请求后，AIDA64会不断回复刷新的数据
         */

        httpClient.setURL("/sse");
        httpCode = httpClient.GET();
        display.print("GET /sse ...\r\n");

        if(httpCode == HTTP_CODE_OK)
        {
            display.print("Succeed!\r\n");

            while (1)
            {
                tcpStream = httpClient.getStreamPtr();
                fd = tcpStream->fd();
                recv_len = recv(fd, httpDataBuffer, 1024, 0);

                if(recv_len <= 0)
                {
                    ESP_LOGW(HTTP_TAG, "Connect error!, recv_len: %d", recv_len);
                    display.print("[HTTP] Disconnect!\r\n");
                    goto END;
                }

                httpDataBuffer[recv_len] = '\0';
                //ESP_LOGI(HTTP_TAG, "%s", payload);

                //parse data
                parseAida64Data(httpDataBuffer, aida64DataList);
                //display data
                display.displayAida64Data(aida64DataList);
            }
        }
        else
        {
            ESP_LOGW(HTTP_TAG, "GET /sse failed, error: %s", httpClient.errorToString(httpCode).c_str());
            display.print("Failed!\r\n");
            goto END;
        }

    END:
        httpClient.end();
        delay(1000);
    }
}

void initHttpClient()
{
    xTaskCreate(taskHttpClient, "taskHttpClient", 102400, NULL, 2, NULL);
}