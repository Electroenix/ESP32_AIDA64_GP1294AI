#ifndef _PUBLIC_H_
#define _PUBLIC_H_

typedef struct
{
    char id[32];
    char val[32];
}AIDA64_DATA;

extern int screen_dir;
extern unsigned long getElapsedTick(unsigned long lastTick);
#endif