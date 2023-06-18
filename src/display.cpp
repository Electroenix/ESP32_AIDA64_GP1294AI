#include "display.h"
#include "config.h"


U8G2_GP1294AI_256X48_F_3W_HW_SPI u8g2(U8G2_R3, /* cs=*/ CS, /* reset=*/ RST);

SCREEN_DISPLAY::SCREEN_DISPLAY()
{
    bufferRaw = 0;
    bufferCol = 0;
}


void SCREEN_DISPLAY::begin()
{
    u8g2.begin();
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setFontPosTop();
}

void SCREEN_DISPLAY::displayAida64Data(std::vector<AIDA64_DATA> &dataList)
{
    u8g2.clearBuffer();

    for(int i = 0; i < dataList.size(); i++)
    {
        Serial.printf("%s\r\n", dataList[i].val);
        u8g2.drawUTF8(0, i * 8, dataList[i].val);
    }
    
    u8g2.sendBuffer();
}

void SCREEN_DISPLAY::clear()
{
    u8g2.clearBuffer();
    bufferRaw = 0;
    bufferCol = 0;
}

void SCREEN_DISPLAY::addString(const char *str, bool multiLine/* = true*/)
{
    int font_h = 0;
    int font_w = 0;
    char c[2] = {0};
    bool nextLine = false;

    font_h = u8g2.getMaxCharHeight();
    font_w = u8g2.getMaxCharWidth();

    do
    {
        //启用多行显示时超出屏幕宽度换行
        if(multiLine && (bufferCol - 1) * font_w > 48)
        {
            bufferCol = 0;
            bufferRaw++;
            nextLine = true;
        }

        c[0] = *str++;
        if(c[0] == '\n')//换行
        {
            if(!nextLine)//如果超出屏幕换行之后紧跟"\n"则不再换行
            {
                bufferCol = 0;
                bufferRaw++;
            }
        }
        else if(c[0] == '\r')//回到行首
        {
            bufferCol = 0;
        }
        else
        {
            u8g2.drawStr(font_w * bufferCol, font_h * bufferRaw, c);
            bufferCol++;
        }

        nextLine = false;
    } while (*str != '\0');

    u8g2.sendBuffer();
}

SCREEN_DISPLAY display;
