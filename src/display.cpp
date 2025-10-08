#include "display.h"
#include "config.h"

#define ROW_HEIGHT (u8g2.getMaxCharHeight() * 1.25)
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
    screen_dir = SCREEN_DIR_VERTICAL;
    is_power_save_mode = false;
    memset(&print_buffer, 0x00, sizeof(print_buffer));
    memset(&cursor, 0x00, sizeof(cursor));
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
    char str_buf[64] = { 0 };

    u8g2.clearBuffer();

    for(int i = 0; i < dataList.size(); i++)
    {
        if(checkAidaItemTitle(dataList[i].val) == 0) // is title
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
    memset(&print_buffer, 0x00, sizeof(print_buffer));
    memset(&cursor, 0x00, sizeof(cursor));
}

void SCREEN_DISPLAY::printBufferAdd(const char *str)
{
    char *data_p = print_buffer.data;
    int *head = &print_buffer.head;
    int *tail = &print_buffer.tail;
    const char *str_p = str;
    int next = 0;
    bool is_empty = 0;

    while(*str_p != '\0')
    {
        if(*head == 0 && *tail == 0)  //buffer is empty
        {
            next = 0;
            is_empty = true;
        }
        else
        {
            next = (*tail + 1) % PRINT_BUFFER_LEN_MAX;
            is_empty = false;
        }

        if(next + strlen(str_p) > PRINT_BUFFER_LEN_MAX)
        {
            //ESP_LOGD(DISPLAY_TAG, "next(%d) + strlen(str_p)(%d) > PRINT_BUFFER_LEN_MAX(%d)", next, strlen(str_p), PRINT_BUFFER_LEN_MAX);
            memcpy(data_p + next, str_p, PRINT_BUFFER_LEN_MAX - (*tail + 1));
            str_p += PRINT_BUFFER_LEN_MAX - next;
            *tail = PRINT_BUFFER_LEN_MAX - 1;
            head = 0;
        }
        else
        {
            //ESP_LOGD(DISPLAY_TAG, "next(%d) + strlen(str_p)(%d) <= PRINT_BUFFER_LEN_MAX(%d)", next, strlen(str_p), PRINT_BUFFER_LEN_MAX);
            memcpy(data_p + next, str_p, strlen(str_p));

            if(*head == next && !is_empty)
            {
                *tail = next - 1 + strlen(str_p);
                *head = (*tail + 1) % PRINT_BUFFER_LEN_MAX;
            }
            else
            {
                *tail = next - 1 + strlen(str_p);
                ESP_LOGD(DISPLAY_TAG, "tail:%d", *tail);
            }

            str_p += strlen(str_p);
            break;
        }
    }
}

void SCREEN_DISPLAY::updateBufferLineIndex()
{
    char *data_p = print_buffer.data;
    int head = print_buffer.head;
    int tail = print_buffer.tail;
    char *c = data_p + head; // head char
    int line_len = 0;
    bool new_line = false;

    line_index_list.clear();
    while(*c != '\0')
    {
        new_line = false;

        if(line_index_list.size() == 0)
        {
            line_len = 0;
            new_line = true;
            ESP_LOGD(DISPLAY_TAG, "new_line, line_index_list.size() == 0");
        }
        else if(*c == '\n') //new line
        {
            line_len = 0;
            new_line = true;
            ESP_LOGD(DISPLAY_TAG, "new_line, c - data_p:%d, c:\\n", c - data_p);
        }
        else if(*c == '\r') //new line head
        {
            line_len = 0;
            ESP_LOGD(DISPLAY_TAG, "c - data_p:%d, c:\\r", c - data_p);
        }
        else
        {
            line_len++;
            if(line_len * COL_WIDTH >= DISPLAY_WIDTH) //line full
            {
                line_len = 0;
                new_line = true;
                ESP_LOGD(DISPLAY_TAG, "new_line, c - data_p:%d, c:%d", c - data_p, c);
            }
        }

        if(new_line)
        {
            line_index_list.push_back(c - data_p + 1);
        }

        /* next char */
        c++;
        if(c - data_p + 1 > PRINT_BUFFER_LEN_MAX)
        {
            c = data_p;
        }

        if(c == data_p + (tail + 1) % PRINT_BUFFER_LEN_MAX)
        {
            break;
        }
    }
}

void SCREEN_DISPLAY::print(const char *str)
{
    const int row_max = DISPLAY_HEIGHT / ROW_HEIGHT;
    char *screen_str = NULL;
    char c[2] = {0};

    printBufferAdd(str);
    updateBufferLineIndex();
    ESP_LOGD(DISPLAY_TAG, "buffer head:%d, buffer tail:%d", print_buffer.head, print_buffer.tail);
    ESP_LOGD(DISPLAY_TAG, "buffer:");
    ESP_LOGD(DISPLAY_TAG, "%s", print_buffer.data);
    ESP_LOGD(DISPLAY_TAG, "line_index_list:");
    for(int i = 0; i < line_index_list.size(); i++)
    {
        ESP_LOGD(DISPLAY_TAG, "%d, ", line_index_list[i]);
    }

    ESP_LOGD(DISPLAY_TAG, "line_index_list.size():%d, row_max:%d, DISPLAY_HEIGHT:%d, ROW_HEIGHT:%f", line_index_list.size(), row_max, DISPLAY_HEIGHT, ROW_HEIGHT);
    if(line_index_list.size() > row_max)
    {
        screen_str = print_buffer.data + line_index_list[line_index_list.size() - row_max];
    }
    else
    {
        screen_str = print_buffer.data;
    }

    cursor.x = 0;
    cursor.y = 0;
    u8g2.clearBuffer();

    do
    {
        //超出屏幕宽度换行
        if(cursor.y * COL_WIDTH >= DISPLAY_WIDTH && c[0] != '\n')
        {
            cursor.y = 0;
            cursor.x++;
        }

        c[0] = *screen_str;
        //ESP_LOGD(DISPLAY_TAG, "c:%s", c);
        if(c[0] == '\n')//换行
        {
            cursor.y = 0;
            cursor.x++;
        }
        else if(c[0] == '\r')//回到行首
        {
            cursor.y = 0;
        }
        else
        {
            u8g2.drawStr(COL_WIDTH * cursor.y, ROW_HEIGHT * cursor.x, c);
            //ESP_LOGD(DISPLAY_TAG, "u8g2.drawStr(%s), (%d, %d)", c, cursor.x, cursor.y);
            cursor.y++;
        }

        /* next char */
        screen_str++;
        //ESP_LOGD(DISPLAY_TAG, "screen_str - print_buffer.data + 1:%d", screen_str - print_buffer.data + 1);
        if(screen_str - print_buffer.data + 1 > PRINT_BUFFER_LEN_MAX)
        {
            screen_str = print_buffer.data;
        }
        //ESP_LOGD(DISPLAY_TAG, "screen_str - print_buffer.data:%d", screen_str - print_buffer.data);
        //ESP_LOGD(DISPLAY_TAG, "screen_str:%c", *screen_str);
        
        if(screen_str == print_buffer.data + (print_buffer.tail + 1) % PRINT_BUFFER_LEN_MAX)
        {
            break;
        }
    } while (*screen_str != '\0');

    u8g2.sendBuffer();
}

SCREEN_DISPLAY display;

void initDisplay()
{
    display.begin(screen_dir);
    ESP_LOGI(DISPLAY_TAG, "Init finish");
    display.clear();
    display.print("Welcome!\r\n");
}