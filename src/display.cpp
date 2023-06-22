#include "display.h"
#include "config.h"

#define ROW_HEIGHT u8g2.getMaxCharHeight()
#define COL_WIDTH u8g2.getMaxCharWidth()
#define DISPLAY_HEIGHT u8g2.getDisplayHeight()
#define DISPLAY_WIDTH u8g2.getDisplayWidth()

U8G2_GP1294AI_256X48_F_3W_HW_SPI u8g2(U8G2_R3, /* cs=*/ CS, /* reset=*/ RST);

SCREEN_DISPLAY::SCREEN_DISPLAY()
{
    bufferRow = 0;
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
        u8g2.drawUTF8(0, i * (ROW_HEIGHT * 1.25), dataList[i].val);
    }
    
    u8g2.sendBuffer();
}

void SCREEN_DISPLAY::clear()
{
    u8g2.clearBuffer();
    bufferRow = 0;
    bufferCol = 0;
}

void SCREEN_DISPLAY::print(const char *str, bool multiLine/* = true*/)
{
    char c[2] = {0};
    bool nextLine = false;

    do
    {
        //启用多行显示时超出屏幕宽度换行
        if(multiLine && bufferCol * COL_WIDTH >= DISPLAY_WIDTH)
        {
            bufferCol = 0;
            bufferRow++;
            nextLine = true;
        }

        c[0] = *str++;
        if(c[0] == '\n')//换行
        {
            if(!nextLine)//如果超出屏幕换行之后紧跟"\n"则不再换行
            {
                bufferCol = 0;
                bufferRow++;
            }
        }
        else if(c[0] == '\r')//回到行首
        {
            bufferCol = 0;
        }
        else
        {
            u8g2.drawStr(COL_WIDTH * bufferCol, ROW_HEIGHT * bufferRow, c);
            bufferCol++;
        }

        nextLine = false;
    } while (*str != '\0');

    u8g2.sendBuffer();
}

SCREEN_DISPLAY display;
