#ifndef APPINFO_H
#define APPINFO_H

#include "draw.h"
#include "graphics.h"

#define MAX_ENTRIES 10
#define BUFFER_SIZE 100
#define DEFAULT_COLOR 0xFFF8AE2D

typedef enum
{
    BOLD = BIT(0),
    SKINNY = BIT(1),
    BIG = BIT(2),
    SMALL = BIT(3),
    CENTER = BIT(4),
    RIGHT_ALIGN = BIT(5),
    NEWLINE = BIT(6),
}               appInfoEntryFlags;

typedef struct  appInfoEntry_s
{
    u32         color;
    u32         flags;
    char        buffer[BUFFER_SIZE];
}               appInfoEntry_t;

void    appInfoInit(sprite_t *sprite);
void    newAppInfoEntry(u32 color, u32 flags, char *text, ...);
void    removeAppInfoEntry(void);
void    clearAppInfo(void);
void    drawAppInfo(void);

#endif