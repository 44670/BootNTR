#include "appInfo.h"

static u32              init = 0;

static sprite_t         *background;
static u32              entryList[BUFFER_SIZE];
static int              entryCount = 0;
static cursor_t         cursor;

void    appInfoInit(sprite_t *sprite)
{
    background = sprite;
    memset(entryList, 0, sizeof(entryList));
    init = 1;
}

void    newAppInfoEntry(u32 color, u32 flags, char *text, ...)
{
    appInfoEntry_t  *entry;
    va_list         vaList;

    if (!text) return;
    if (entryCount >= MAX_ENTRIES) return;
    entry = (appInfoEntry_t *)calloc(1, sizeof(appInfoEntry_t));
    if (!entry) return;
    va_start(vaList, text);
    vsnprintf(entry->buffer, BUFFER_SIZE, text, vaList);
    va_end(vaList);
    entry->color = color;
    entry->flags = flags;
    entryList[entryCount] = (u32)entry;
    entryCount++;
    updateUI();
}

void    removeAppInfoEntry(void)
{
    appInfoEntry_t *entry;

    if (entryCount <= 0) return;
    entryCount--;
    entry = (appInfoEntry_t *)entryList[entryCount];
    entryList[entryCount] = 0;
    free(entry);
    updateUI();
}

void    clearAppInfo(void)
{
    int     i;

    for (i = entryCount; i > 0; i--)
        removeAppInfoEntry();
}

static void getDrawParameters(appInfoEntry_t *entry, float *sizeX, float *sizeY)
{
    float   textWidth;
    float   scaleX;
    float   scaleY;
    float   temp;
    u32     flags;

    if (!entry | !sizeX | !sizeY) return;
    flags = entry->flags;
    //Set the font size
    if (flags & BIG) scaleX = scaleY = 0.6f;
    else if (flags & SMALL) scaleX = scaleY = 0.4f;
    else scaleX = scaleY = 0.5f;

    //Set the type
    if (flags & BOLD) scaleX += 0.05f;
    else if (flags & SKINNY) scaleY += 0.05f;

    //Set the alignment
    getTextSizeInfos(&textWidth, scaleX, scaleY, entry->buffer);
    if (flags & CENTER)
    {
        temp = 297.0f - cursor.posX;
        temp -= textWidth;
        if (temp > 0)
        {
            temp /= 2;
            cursor.posX += temp;
        }
    }
    if (flags & RIGHT_ALIGN)
    {
        cursor.posX = 297.0f;
        cursor.posX -= textWidth;
    }

    if (flags & NEWLINE)
        cursor.posY += 0.4f * fontGetInfo()->lineFeed;
    
    //Return the size
    *sizeX = scaleX;
    *sizeY = scaleY;
}

void    drawAppInfoEntry(int index)
{
    appInfoEntry_t  *entry;
    float           sizeX;
    float           sizeY;
    float           lineFeed;

    if (index >= entryCount || index < 0) goto exit;
    entry = (appInfoEntry_t *)entryList[index];
    sizeX = sizeY = 0.0f;
    getDrawParameters(entry, &sizeX, &sizeY);
    lineFeed = sizeY * fontGetInfo()->lineFeed;
#ifndef CITRA
    setTextColor(entry->color);
#endif
    renderText(cursor.posX, cursor.posY, sizeX, sizeY, false, entry->buffer, &cursor);
    cursor.posY += lineFeed;
exit:
    return;
}

void    drawAppInfo(void)
{
    int     i;

    if (!init || entryCount <= 0) return;

    cursor = (cursor_t){ 177, 64};
    drawSprite(background);
    for (i = 0; i < entryCount; i++)
    {
        if (cursor.posY >= 190) break;
        cursor.posX = 177;
        drawAppInfoEntry(i);
    }
}
