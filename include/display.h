#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include "U8g2lib.h"
#include "public.h"
#include <vector>

extern U8G2_GP1294AI_256X48_F_3W_HW_SPI u8g2;

class SCREEN_DISPLAY
{
public:
    SCREEN_DISPLAY();

public:
    void begin();
    void displayAida64Data(std::vector<AIDA64_DATA> &dataList);
    void clear();
    void addString(const char *str, bool multiLine = true);

private:
    int bufferRaw;
    int bufferCol;
};

extern SCREEN_DISPLAY display;
#endif