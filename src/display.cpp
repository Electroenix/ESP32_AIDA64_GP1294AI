#include "display.h"
#include "http_client.h"
#include "config.h"

U8G2_GP1294AI_256X48_F_3W_HW_SPI u8g2(U8G2_R3, /* cs=*/ CS, /* reset=*/ RST);

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
