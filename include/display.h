#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include <U8g2lib.h>
#include "public.h"
#include <vector>
#include "trace.h"

enum
{
    SCREEN_DIR_HORIZONTAL,
    SCREEN_DIR_VERTICAL,
    SCREEN_DIR_MAX,
} SCREEN_DIR;

typedef struct position_tag
{
    int x;
    int y;
} position_t;

typedef struct
{
    char* data;
    int row;
    int col;
}ScreenBuffer_t;


class FifoBuffer
{
public:
    char *data; /* 缓冲区数组 */
    int head; /* 第一个元素的index */
    int tail; /* 最后一个元素的下一个位置的index */
    int insert; /* 下次写入数据位置 */
    int capacity; /* 容量, 比实际可存储有效数据多1 */
    int count; /* 有效元素个数 */

    FifoBuffer() : data(nullptr), head(0), tail(0), insert(0), count(0), capacity(0) {}
    FifoBuffer(const FifoBuffer&) = delete;
    FifoBuffer& operator=(const FifoBuffer&) = delete;

    ~FifoBuffer()
    { 
        if (data)
        {
            free(data);
            data = NULL;
        }
    }

    bool create(int len)
    {
        if (data) {
            free(data);
        }

        data = (char *)malloc(len + 1); // 比实际有效容量大1，保证buffer满后，tail在空位置，head在tail的下一个位置
        if (!data) {
            return false;
        }

        capacity = len + 1;
        clear();
        return true;
    }

    void write_impl(const char *str, int len);
    void write(const char *str);
    void write(const char chr);

    bool set_insert(int pos)
    {
        if(pos < 0 || pos >= capacity) return false;
        insert = pos;
        return true;
    }

    bool move_insert(int offset)
    {
        if(offset < -1 * capacity || offset > capacity) return false;
        insert = (insert + offset + capacity) % capacity;
        return true;
    }

    void clear()
    {
        if(data)
        {
            memset(data, 0x00, capacity);
        }
        head = 0;
        tail = 0;
        insert = 0;
        count = 0;
    }

    bool is_empty()
    {
        return count == 0;
    }

    bool is_full()
    {
        return count == capacity - 1;
    }

    /* 相对位置 */
    int rel_pos(int origin, int index)
    {
        if(index < -1 * capacity || index > capacity) return 0;
        return (index + capacity - origin) % capacity;
    }

    /* 相对buffer首地址的index */
    int abs_pos(int index)
    {
        return rel_pos(0, index);
    }

    /* 相对head位置 */
    int rel_head_pos(int index)
    {
        return rel_pos(head, index);
    }

    /* 根据相对buffer首地址的绝对索引遍历数据 */
    void abs_traverse(int begin, int end, void (*func)(char*, void*), void *arg)
    {
        int index = begin;

        while(data[index] != '\0')
        {
            if(func)
            {
                func(&data[index], arg);
            }

            /* next char */
            index = abs_pos(index + 1);

            if(index == tail || index == end)
            {
                break;
            }
        }
    }
};

class MultiLineTextBuffer
{
public:
    FifoBuffer data_buf;
    std::vector<int> line_heads; /* 记录行首位置 */
    int max_row;                 /* 行数 */
    int max_col;                 /* 列数 */
    int line_len;           /* 记录当前行数据长度 */

    void create(int max_len, int max_row, int max_col)
    {
        this->max_row = max_row;
        this->max_col = max_col;
        line_len = 0;
        data_buf.create(max_len);
    }
    void write(const char *str);
    void clear()
    {
        data_buf.clear();
        line_heads.clear();
        line_len = 0;
    }
};

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

    void setPowerSaveMode(bool is_enable)
    {
        u8g2.setPowerSave(is_enable);
        is_power_save_mode = is_enable;
    }
    bool getPowerSaveMode()
    {
        return is_power_save_mode;
    }

private:
    U8G2_GP1294AI_256X48_F_4W_HW_SPI u8g2;
    int screen_dir;
    MultiLineTextBuffer text_buffer; /* 打印字符串缓冲区 */
    ScreenBuffer_t screen_buffer; /* 屏幕显示数据缓冲区 */
    bool is_power_save_mode;

    void refreshScreenBuffer();
};

class LoadingString
{
public:
    LoadingString(const char *str = "");
    String step();

private:
    String string;
    char load_chars[8];
    int index;
};

extern SCREEN_DISPLAY display;

void initDisplay();
#endif