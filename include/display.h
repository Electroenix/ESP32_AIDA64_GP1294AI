#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include "U8g2lib.h"
#include "public.h"
#include <vector>

enum
{
    SCREEN_DIR_HORIZONTAL,
    SCREEN_DIR_VERTICAL,
    SCREEN_DIR_MAX,
}SCREEN_DIR;

class SCREEN_DISPLAY
{
public:
    SCREEN_DISPLAY();

public:
    void begin(int dir);
    void setScreenDir(int dir);
    void displayAida64Data(std::vector<AIDA64_DATA> &dataList);
    void displayAida64Data_vertical(std::vector<AIDA64_DATA> &dataList);
    void displayAida64Data_horizontal(std::vector<AIDA64_DATA> &dataList);
    void clear();
    void print(const char *str, bool multiLine = true);
    void setPowerSave(uint8_t is_enable) {
      u8g2.setPowerSave(is_enable); }

private:
    U8G2_GP1294AI_256X48_F_3W_HW_SPI u8g2;
    int screen_dir;
    int bufferRow;
    int bufferCol;
};

extern SCREEN_DISPLAY display;
#endif