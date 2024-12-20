# ESP32_AIDA64_GP1294AI
本项目基于ESP32，可以实现无线方式在VFD显示屏上显示电脑AIDA64监控数据

## 环境
+ VSCODE + platform IDE
  > Board选择Espressif ESP32 Dev Module，Framework选择Arduino
+ 控制器型号ESP32-WROOM-32
+ VFD显示屏GP1294AI 256x48
+ AIDA64

## 使用
1. 打开AIDA64软件，打开设置->LCD->RemoteSensor, 输入一个端口号，勾选启用RemoteSensor LCD支持，然后点击应用或OK

2. 选择LCD项目，将aida64config/example.rslcd导入，点击OK

3. 可以在浏览器里输入本地ip和端口号看看是否能显示面板，最好在局域网内用其他设备访问验证，确保没有被防火墙屏蔽

4. 配置config.h中的WIFI账密，HTTP地址和端口号
    ```
    //WIFI
    #define WIFI_SSID "SSID"
    #define WIFI_PASS "PASS"
    
    //HTTP
    #define HTTP_HOST "HOST"
    #define HTTP_PORT 8080
    ```
    >确保ESP32和电脑连接的是同一个wifi，HOST是电脑无线网卡的ip地址
    >
    >注：ESP32只能连接2.4GHz频段的WIFI，5G或者双频段都可能连不上

5. 连接ESP32和OLED，我使用的ESP32模块SPI引脚为
    ```
    GPIO23 -> MOSI
    GPIO19 -> MISO // 这里不用
    GPIO18 -> CLK
    GPIO5  -> CS
    ```

    如果不是相同模块可能引脚定义不同，需要自行在config.h中更改，RST可以随便定义一个GPIO就行
    ```
    /* 
    * GP1294AI PIN define
    * use 3 wire SPI
    * SCL -> 18
    * SDA -> 23
    */
    #define CS 5
    #define RST 21
    ```

    VFD显示屏接口查看规格书，其中CLOCK，CHIPSELECT，DATA，是SPI引脚的CLK，CS，MOSI，与上面ESP32的对应引脚连接，如果规格和定义都和我的一样，那么连接应该如下：
    ```
    VFD              ESP32
    -----------------------
    FILMENT_EN    -> 3V3
    CLOCK         -> GPIO18
    CHIPSELECT    -> GPIO5
    DATA          -> GPIO23
    RESET         -> GPIO21
    GND           -> GND
    ```

    这块VFD还有个+5V OUT接口可以输出电压，可以将其与ESP32的VIN连接，给ESP32供电，这样只要连接VFD的电源线就可以同时给屏幕和ESP32提供电源

6. 连接完成后，编译下载到ESP32，即可使用
  
