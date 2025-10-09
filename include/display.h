#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include <U8g2lib.h>
#include "public.h"
#include <vector>
#include "trace.h"

#define PRINT_BUFFER_LEN_MAX 2048

enum
{
    SCREEN_DIR_HORIZONTAL,
    SCREEN_DIR_VERTICAL,
    SCREEN_DIR_MAX,
}SCREEN_DIR;

typedef struct position_tag
{
    int x;
    int y;
}position_t;

typedef struct print_buffer_tag
{
    char data[PRINT_BUFFER_LEN_MAX + 1];
    int head;
    int tail;

    void traverse(int begin, int end, void (*func)(char*, void*), void *arg)
    {
        char *c = this->data + begin;

        while(*c != '\0')
        {
            if(func)
            {
                func(c, arg);
            }

            /* next char */
            c++;
            if(c - this->data + 1 > PRINT_BUFFER_LEN_MAX)
            {
                c = this->data;
            }

            if(c == this->data + (end + 1) % PRINT_BUFFER_LEN_MAX)
            {
                break;
            }
        }
    }
}print_buffer_t;

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
    void print(const char *str);

    void setPowerSaveMode(bool is_enable) {
        u8g2.setPowerSave(is_enable);
        is_power_save_mode = is_enable;
    }
    bool getPowerSaveMode() {
        return is_power_save_mode;
    }

private:
    U8G2_GP1294AI_256X48_F_4W_HW_SPI u8g2;
    int screen_dir;
    print_buffer_t print_buffer;  /* fifo buffer, 存储打印数据 */
    std::vector<int> line_index_list;  /* 用来存储buffer中每行数据开头第一个字符在buffer中的位置 */
    position_t cursor;  /* 下一次打印数据的位置 */
    bool is_power_save_mode;

    void printBufferAdd(const char *str);
    void updateBufferLineIndex();
};

extern SCREEN_DISPLAY display;

void initDisplay();
#endif