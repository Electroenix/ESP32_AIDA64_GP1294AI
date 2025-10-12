#include "display.h"
#include "config.h"

#define ROW_HEIGHT (u8g2.getMaxCharHeight() * 1.25)
#define COL_WIDTH u8g2.getMaxCharWidth()
#define DISPLAY_HEIGHT u8g2.getDisplayHeight()
#define DISPLAY_WIDTH u8g2.getDisplayWidth()
#define PRINT_BUFFER_LEN_MAX 1024

const char *aida_item_title[] = {"TIME", "CPU", "GPU", "MEM", "NET"};

int checkAidaItemTitle(const char *str)
{
    const int len = sizeof(aida_item_title) / sizeof(aida_item_title[0]);
    int i = 0;

    for (i = 0; i < len; i++)
    {
        if (strcmp(str, aida_item_title[i]) == 0)
        {
            return 0;
        }
    }

    return -1;
}

void FifoBuffer::write_impl(const char *str, int len)
{
    const char *str_p = str;
    int tmp_wrt_len = 0;
    int cover_len = 0;
    int add_len = 0;
    int old_insert = 0;
    int remaining = len;

    if (is_empty()) // buffer is empty
    {
        head = 0;
        tail = 0;
        insert = 0;
    }

    while (remaining > 0)
    {
        if (insert + remaining > capacity - 1)
        {
            /* 写入数据溢出buffer，则写至buffer末尾，insert移动到buffer开头，剩余数据进入下次循环 */
            tmp_wrt_len = capacity - insert;
        }
        else
        {
            /* 写入数据未溢出buffer，直接写入 */
            tmp_wrt_len = remaining;
        }

        memcpy(data + insert, str_p, tmp_wrt_len);

        old_insert = insert;
        insert = abs_pos(insert + tmp_wrt_len);

        /* insert位置若在tail之前，则一部分末尾数据被覆盖，计算覆盖数据长度 */
        cover_len = 0;
        if (old_insert < tail)
        {
            cover_len = std::min(insert, tail) - old_insert;
        }
        add_len = tmp_wrt_len - cover_len;

        if (tmp_wrt_len >= rel_pos(old_insert, tail))
        {
            /* 写入长度超过old_insert到tail的长度(即写入后insert超过tail)，则同步更新tail */
            tail = insert;
        }

        if (is_full())
        {
            /* 缓冲区已满，旧数据被覆盖，更新head，由于新数据覆盖旧数据，count不变 */
            head = abs_pos(tail + 1);
        }
        else
        {
            count += add_len;
        }

        str_p += tmp_wrt_len;
        remaining -= tmp_wrt_len;

        ESP_LOGI(DISPLAY_TAG, "head: %d, tail:%d, insert:%d, count:%d, capacity:%d", head, tail, insert, count, capacity);
    }
}

void FifoBuffer::write(const char *str)
{
    if (!str)
        return;
    write_impl(str, strlen(str));
}

void FifoBuffer::write(const char chr)
{
    write_impl(&chr, 1);
}

void MultiLineTextBuffer::write(const char *str)
{
    const char *c_p = str;
    bool new_line = false;

    ESP_LOGI(DISPLAY_TAG, "str: %s", str);
    while (*c_p != '\0')
    {
        // ESP_LOGI(DISPLAY_TAG, "*c_p: %c", *c_p);
        new_line = false;

        if (line_heads.empty())
        {
            /* 第一个字符 */
            line_len = 0;
            line_heads.push_back(data_buf.tail);
            // ESP_LOGD(DISPLAY_TAG, "new_line, line_index_list.size() == 0");
        }

        data_buf.write(*c_p);

        /* 计算行首信息 */
        if (*c_p == '\b')
        {
            if(line_len > 0)
                line_len--;
        }
        else if (*c_p == '\r')
        {
            line_len = 0;
        }
        else if (*c_p == '\n')
        {
            line_len = 0;
            new_line = true;
        }
        else
        {
            line_len++;
            if (line_len > max_col) // line full
            {
                // ESP_LOGI(DISPLAY_TAG, "line_len: %d", line_len);
                line_len = 0;
                new_line = true;
            }
        }

        if (!line_heads.empty() && line_heads[0] == data_buf.tail)
        {
            //插入字符覆盖了第一行，删除第一行
            line_heads.erase(line_heads.begin());
            // ESP_LOGI(DISPLAY_TAG, "delete line head: %d", data_buf.tail);
        }

        if (new_line)
        {
            line_heads.push_back(data_buf.tail);
            // ESP_LOGI(DISPLAY_TAG, "new line head: %d", data_buf.tail);
        }

        /* next char */
        c_p++;
    }
}

SCREEN_DISPLAY::SCREEN_DISPLAY() : u8g2(U8G2_R3, /* cs=*/CS, /* dc=*/U8X8_PIN_NONE, /* reset=*/RST)
{
    screen_dir = SCREEN_DIR_VERTICAL;
    is_power_save_mode = false;
}

void SCREEN_DISPLAY::begin(int dir)
{
    u8g2.begin();
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setFontPosTop();
    u8g2.setContrast(70);
    setScreenDir(dir);

    text_buffer.create(PRINT_BUFFER_LEN_MAX, DISPLAY_HEIGHT / ROW_HEIGHT, DISPLAY_WIDTH / COL_WIDTH);
    screen_buffer.row = text_buffer.max_row;
    screen_buffer.col = text_buffer.max_col;
    screen_buffer.data = (char *)malloc(screen_buffer.row * screen_buffer.col + 1);
    memset(screen_buffer.data, 0x00, screen_buffer.row * screen_buffer.col + 1);
}

void SCREEN_DISPLAY::setScreenDir(int dir)
{
    screen_dir = dir;

    if (screen_dir == SCREEN_DIR_VERTICAL)
    {
        u8g2.setDisplayRotation(U8G2_R3);
    }
    else if (screen_dir == SCREEN_DIR_HORIZONTAL)
    {
        u8g2.setDisplayRotation(U8G2_R2);
    }
}

void SCREEN_DISPLAY::displayAida64Data_vertical(std::vector<AIDA64_DATA> &dataList)
{
    int u8g2_ret = 0;
    int pos_y = 0;
    char str_buf[64] = {0};

    u8g2.clearBuffer();

    for (int i = 0; i < dataList.size(); i++)
    {
        if (checkAidaItemTitle(dataList[i].val) == 0) // is title
        {
            pos_y++;
            sprintf(str_buf, "%s", dataList[i].val);
            u8g2_ret = u8g2.drawUTF8(0, pos_y * ROW_HEIGHT, str_buf);
            pos_y++;
        }
        else // is not title
        {
            sprintf(str_buf, ">%s", dataList[i].val);
            u8g2_ret = u8g2.drawUTF8(0, pos_y * ROW_HEIGHT, str_buf);
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
    char str_buf[64] = {0};

    u8g2.clearBuffer();

    for (int i = 0; i < dataList.size(); i++)
    {
        if (checkAidaItemTitle(dataList[i].val) == 0) // is title
        {
            pos_x = 0;
            pos_y++;
            sprintf(str_buf, "%-5s", dataList[i].val);
            u8g2_ret = u8g2.drawUTF8(pos_x, pos_y * ROW_HEIGHT, str_buf);
            pos_x += COL_WIDTH * strlen(str_buf);
        }
        else // is not title
        {
            sprintf(str_buf, " |   %s  ", dataList[i].val);
            u8g2_ret = u8g2.drawUTF8(pos_x, pos_y * ROW_HEIGHT, str_buf);
            pos_x += COL_WIDTH * strlen(str_buf);
        }
    }

    u8g2.sendBuffer();
}

void SCREEN_DISPLAY::displayAida64Data(std::vector<AIDA64_DATA> &dataList)
{
    if (screen_dir == SCREEN_DIR_VERTICAL)
    {
        displayAida64Data_vertical(dataList);
    }
    else if (screen_dir == SCREEN_DIR_HORIZONTAL)
    {
        displayAida64Data_horizontal(dataList);
    }
}

void SCREEN_DISPLAY::clear()
{
    u8g2.clearBuffer();
    text_buffer.clear();
    memset(screen_buffer.data, 0x00, screen_buffer.row * screen_buffer.col);
}

void SCREEN_DISPLAY::refreshScreenBuffer()
{
    int start = 0;
    position_t cursor = { 0 }; /* cursor.x: ->   cursor.y: ↓ */

    if (!screen_buffer.data || text_buffer.line_heads.empty())
        return;

    memset(screen_buffer.data, ' ', screen_buffer.row * screen_buffer.col);

    start = text_buffer.line_heads[0];
    if (text_buffer.line_heads.size() > text_buffer.max_row)
    {
        /* 记录的行比总行数多，截取后面的行 */
        start = text_buffer.line_heads[text_buffer.line_heads.size() - text_buffer.max_row];
    }

    int index = start;
    while(text_buffer.data_buf.data[index] != '\0')
    {
        //ESP_LOGI(DISPLAY_TAG, "*c_p: %c", *c_p);
        if (text_buffer.data_buf.data[index] == '\b') //退格
        {
            if(cursor.x > 0)
                cursor.x--;
        }
        else if (text_buffer.data_buf.data[index] == '\r') //回车
        {
            cursor.x = 0;
        }
        else if (text_buffer.data_buf.data[index] == '\n') //换行
        {
            cursor.x = 0;
            cursor.y++;

            // ESP_LOGI(DISPLAY_TAG, "new_line, x:%d, y:%d", cursor.x, cursor.y);
        }
        else
        {
            int pos = cursor.x + (cursor.y * screen_buffer.col);
            if(pos <= screen_buffer.row * screen_buffer.col)
            {
                screen_buffer.data[cursor.x + (cursor.y * screen_buffer.col)] = text_buffer.data_buf.data[index];
                cursor.x++;
            }

            if (cursor.x > screen_buffer.col - 1) // line full
            {
                cursor.x = 0;
                cursor.y++;
            }
        }

        /* next char */
        index = text_buffer.data_buf.abs_pos(index + 1);

        if(index == text_buffer.data_buf.tail)
        {
            break;
        }
    }
}

void SCREEN_DISPLAY::print(const char *str)
{
    text_buffer.write(str);
    ESP_LOGI(DISPLAY_TAG, "text_buffer.line_heads.size(): %d", text_buffer.line_heads.size());

    refreshScreenBuffer();
    ESP_LOGI(DISPLAY_TAG, "screen_buffer: %s<<<<<<<<<<<<<<<<<", screen_buffer);

    u8g2.clearBuffer();

    for (int row = 0; row < screen_buffer.row; row++)
    {
        char line_buf[screen_buffer.col + 1];
        memcpy(line_buf, &screen_buffer.data[row * screen_buffer.col], screen_buffer.col);
        line_buf[screen_buffer.col] = '\0';

        u8g2.drawStr(0, row * ROW_HEIGHT, line_buf);
    }

    u8g2.sendBuffer();
}

/* 显示加载状态的字符串，通过step()更新状态 */
LoadingString::LoadingString(const char *str /*= ""*/)
{
    index = 0;
    memcpy(load_chars, "/-\\|/-\\|", 8);
    string = String(str);
}

String LoadingString::step()
{
    String ret_str;

    if (index == 0)
    {
        ret_str = string + load_chars[index % 8];
    }
    else
    {
        ret_str = String("\b", string.length() + 1) + string + load_chars[index % 8];
    }

    index++;

    return ret_str;
}

SCREEN_DISPLAY display;

void initDisplay()
{
    display.begin(screen_dir);
    ESP_LOGI(DISPLAY_TAG, "Init finish");
    display.clear();
    display.print("Welcome!\r\n");
}