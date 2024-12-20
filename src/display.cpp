#include "display.h"
#include "config.h"

#define ROW_HEIGHT u8g2.getMaxCharHeight()
#define COL_WIDTH u8g2.getMaxCharWidth()
#define DISPLAY_HEIGHT u8g2.getDisplayHeight()
#define DISPLAY_WIDTH u8g2.getDisplayWidth()

const char *aida_item_title[] = { "TIME", "CPU", "GPU", "MEM", "NET" };

int checkAidaItemTitle(const char *str)
{
    const int len = sizeof(aida_item_title) / sizeof(aida_item_title[0]);
    int i = 0;

    for(i = 0; i < len; i++)
    {
        if(strcmp(str, aida_item_title[i]) == 0)
        {
            return 0;
        }
    }

    return -1;
}

SCREEN_DISPLAY::SCREEN_DISPLAY():u8g2(U8G2_R3, /* cs=*/ CS, /* reset=*/ RST)
{
    bufferRow = 0;
    bufferCol = 0;
    screen_dir = SCREEN_DIR_VERTICAL;
}

void SCREEN_DISPLAY::begin(int dir)
{
    u8g2.begin();
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setFontPosTop();
    setScreenDir(dir);
}

void SCREEN_DISPLAY::setScreenDir(int dir)
{
    screen_dir = dir;

    if(screen_dir == SCREEN_DIR_VERTICAL)
    {
        u8g2.setDisplayRotation(U8G2_R3);
    }
    else if(screen_dir == SCREEN_DIR_HORIZONTAL)
    {
        u8g2.setDisplayRotation(U8G2_R2);
    }
}

void SCREEN_DISPLAY::displayAida64Data_vertical(std::vector<AIDA64_DATA> &dataList)
{
    int u8g2_ret = 0;
    int pos_y = 0;
    char str_buf[64] = { 0 };

    u8g2.clearBuffer();

    for(int i = 0; i < dataList.size(); i++)
    {
        if(checkAidaItemTitle(dataList[i].val) == 0) // is title
        {
            pos_y++;
            sprintf(str_buf, "%s", dataList[i].val);
            u8g2_ret = u8g2.drawUTF8(0, pos_y * (ROW_HEIGHT * 1.25), str_buf);
            pos_y++;
        }
        else // is not title
        {
            sprintf(str_buf, ">%s", dataList[i].val);
            u8g2_ret = u8g2.drawUTF8(0, pos_y * (ROW_HEIGHT * 1.25), str_buf);
            pos_y++;
        }
    }
    
    u8g2.sendBuffer();
}

void SCREEN_DISPLAY::displayAida64Data_horizontal(std::vector<AIDA64_DATA> &dataList)
{
    int u8g2_ret = 0;
    int pos_x = 0;
    int pos_y = -1;
    char str_buf[64] = { 0 };

    u8g2.clearBuffer();

    for(int i = 0; i < dataList.size(); i++)
    {
        if(checkAidaItemTitle(dataList[i].val) == 0) // is title
        {
            pos_x = 0;
            pos_y++;
            sprintf(str_buf, "%-5s", dataList[i].val);
            u8g2_ret = u8g2.drawUTF8(pos_x, pos_y * (ROW_HEIGHT * 1.25), str_buf);
            pos_x += COL_WIDTH * strlen(str_buf);
        }
        else // is not title
        {
            sprintf(str_buf, " |   %s  ", dataList[i].val);
            u8g2_ret = u8g2.drawUTF8(pos_x, pos_y * (ROW_HEIGHT * 1.25), str_buf);
            pos_x += COL_WIDTH * strlen(str_buf);
        }
    }
    
    u8g2.sendBuffer();
}

void SCREEN_DISPLAY::displayAida64Data(std::vector<AIDA64_DATA> &dataList)
{
    if(screen_dir == SCREEN_DIR_VERTICAL)
    {
        displayAida64Data_vertical(dataList);
    }
    else if(screen_dir == SCREEN_DIR_HORIZONTAL)
    {
        displayAida64Data_horizontal(dataList);
    }
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

    do
    {
        //启用多行显示时超出屏幕宽度换行
        if(multiLine && bufferCol * COL_WIDTH >= DISPLAY_WIDTH && c[0] != '\n')
        {
            bufferCol = 0;
            bufferRow++;
        }

        c[0] = *str++;
        if(c[0] == '\n')//换行
        {
            bufferCol = 0;
            bufferRow++;
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

    } while (*str != '\0');

    u8g2.sendBuffer();
}

SCREEN_DISPLAY display;
