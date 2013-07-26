#include "loader.h"

static unsigned char *SCREEN_BASE = (char *)0xB8000;

int putchar(int c)
{
    unsigned int curpos, i;

    outportb(0x3d4, 0x0e);
    curpos = inportb(0x3d5);
    curpos <<= 8;
    outportb(0x3d4, 0x0f);
    curpos += inportb(0x3d5);
    curpos <<= 1;

    switch(c) {
    case '\n':
        curpos = (curpos/160)*160 + 160;
        break;

    case '\r':
        curpos = (curpos/160)*160;
        break;

    case '\t':
	    curpos += 8;
    	break;

    case '\b':
    	curpos -= 2;
    	SCREEN_BASE[curpos] = 0x20;
    	break;

    default:
        SCREEN_BASE[curpos++] = c;
        SCREEN_BASE[curpos++] = 0x07;
        break;
    }

    if(curpos >= 160*25) {
        for(i = 0; i < 160*24; i++) {
           SCREEN_BASE[i] = SCREEN_BASE[i+160];
        }
        for(i = 0; i < 80; i++) {
           SCREEN_BASE[(160*24)+(i*2)  ] = 0x20;
           SCREEN_BASE[(160*24)+(i*2)+1] = 0x07;
        }
        curpos -= 160;
    }

    curpos >>= 1;
    outportb(0x3d4, 0x0f);
    outportb(0x3d5, curpos & 0x0ff);
    outportb(0x3d4, 0x0e);
    outportb(0x3d5, curpos >> 8);
    
    return 1;
}

