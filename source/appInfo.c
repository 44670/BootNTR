#include "appInfo.h"
#include "graphics.h"

static bool autoUpdate = false;
static bool showBackground = true;
extern appInfoObject_t  *appTop;

void    appInfoDisableAutoUpdate(void)
{ 
    autoUpdate = false;
}

void    appInfoEnableAutoUpdate(void)
{
    autoUpdate = true;
}

void    appInfoHideBackground(void)
{
    showBackground = false;
}

void    appInfoShowBackground(void)
{
    showBackground = true;
}

appInfoObject_t     *newAppInfoObject(sprite_t *sprite, u32 maxEntryCount, u32 posX, u32 posY)
{
    appInfoObject_t *object;

    if (!sprite) goto error;

    object = (appInfoObject_t *)calloc(1, sizeof(appInfoObject_t));
    if (!object) goto error;
    object->entryList = (u32 *)calloc(maxEntryCount, sizeof(u32 *));
    if (!object->entryList) goto allocError;
    object->sprite = sprite;
    object->spritePosX = sprite->posX;
    object->spritePosY = sprite->posY;
    object->maxEntryCount = maxEntryCount;
    object->entryCount = 0;
    object->cursor.posX = (float)posX;
    object->cursor.posY = (float)posY;
    object->boundX = 0;
    object->boundY = 0;
    return (object);
allocError:
    free(object);
error:
    return (NULL);
}

void    deleteAppInfoObject(appInfoObject_t *object)
{
    if (!object) return;
    free(object->entryList);
    free(object);
}

void    appInfoSetTextBoundaries(appInfoObject_t *object, float posX, float posY)
{

    if (!object) return;
    object->boundX = posX;
    object->boundY = posY;
}

void    appInfoSetSpritePosition(appInfoObject_t *object, float posX, float posY)
{
    if (!object) return;
    object->spritePosX = posX;
    object->spritePosY = posY;
}

static void scrollDown(appInfoObject_t *object)
{
    appInfoEntry_t  *entry;
    int             index;
    u32             entryCount;
    u32             *entryList;

    if (!object) goto exit;
    entryCount = object->entryCount;
    if (entryCount <= 0) goto exit;
    entryList = object->entryList;
    entry = (appInfoEntry_t *)entryList[0];
    free(entry);
    entryCount--;
    for (index = 0; index < entryCount; index++)
    {
        entryList[index] = entryList[index + 1];
    }
    entryList[index] = 0;
    object->entryCount = entryCount;
exit:
    return;
}

void newAppInfoEntry(appInfoObject_t *object, u32 color, u32 flags, char *text, ...)
{
    u32             entryCount;
    u32             *entryList;
    appInfoEntry_t  *entry;
    va_list         vaList;

    if (!text || !object) goto exit;
    if (flags & SCROLL)
    {
        scrollDown(object);
        if (flags & NEWLINE)
            scrollDown(object);
    }
    entryCount = object->entryCount;
    entryList = object->entryList;
    if (entryCount >= object->maxEntryCount)
    {
        scrollDown(object);
        entryCount = object->entryCount;
    }
    entry = (appInfoEntry_t *)calloc(1, sizeof(appInfoEntry_t));
    if (!entry) goto exit;
    va_start(vaList, text);
    vsnprintf(entry->buffer, BUFFER_SIZE, text, vaList);
    va_end(vaList);
    entry->color = color;
    entry->flags = flags;
    entryList[entryCount] = (u32)entry;
    object->entryCount++;
    if (autoUpdate)
        updateUI();
exit:
    return;
}

static void deleteLastEntry(appInfoObject_t *object)
{
    u32             entryCount;
    appInfoEntry_t  *entry;

    if (!object) goto exit;
    entryCount = object->entryCount;
    if (entryCount <= 0) goto exit;
    entryCount--;
    entry = (appInfoEntry_t *)object->entryList[entryCount];
    object->entryList[entryCount] = 0;
    free(entry);
    object->entryCount = entryCount;
exit:
    return;

}

void    removeAppInfoEntry(appInfoObject_t *object)
{
    deleteLastEntry(object);
    if (autoUpdate)
        updateUI();
}

void    clearAppInfo(appInfoObject_t *object, bool updateScreen)
{
    u32     entryCount;
    int     i;

    entryCount = object->entryCount;
    for (i = entryCount; i > 0; i--)
        deleteLastEntry(object);
    if (updateScreen)
        updateUI();
}

void drawMultilineText(u32 color, u32 flags, char* txt) {
	float textWidth;
	float totalWidth = appTop->boundX - appTop->cursor.posX;
	float scaleX, scaleY;
	//Set the font size
	if (flags & BIG) scaleX = scaleY = 0.6f;
	else if (flags & MEDIUM) scaleX = scaleY = 0.55f;
	else if (flags & SMALL) scaleX = scaleY = 0.45f;
	else if (flags & TINY) scaleX = scaleY = 0.4f;
	else scaleX = scaleY = 0.5f;

	int textLen = strlen(txt) + 1;
	char* copy = malloc(textLen);
	char* copyCurr = copy;
	memset(copy, 0, textLen);
	char* breakpos = copy;
	char* txtCurr = txt;
	while (*txtCurr != '\0') {
		*copyCurr = *txtCurr;
		if (*copyCurr == ' ') breakpos = copyCurr;
		getTextSizeInfos(&textWidth, scaleX, scaleY, copy);
		if (textWidth >= totalWidth) {
			if (breakpos == copy) {
				*(copyCurr - 1) = '\0';
				newAppTop(color, flags, copy);
				memset(copy, 0, textLen);
				copyCurr = copy;
				breakpos = copy;
				txtCurr--;
			}
			else {
				*breakpos = '\0';
				newAppTop(color, flags, copy);
				memset(copy, 0, textLen);
				txtCurr = (txtCurr - (copyCurr - breakpos)) + 1;
				copyCurr = copy;
				breakpos = copy;
			}
		}
		else {
			copyCurr++;
			txtCurr++;
		}
	}
	if (copy != copyCurr) {
		newAppTop(color, flags, copy);
	}
	free(copy);
}

static void getDrawParameters(appInfoObject_t *object, int index, float *sizeX, float *sizeY)
{
    appInfoEntry_t  *entry;
    float           textWidth;
    float           scaleX;
    float           scaleY;
    float           temp;
    u32             flags;
    cursor_t        *cursor;

    if (!object || !sizeX || !sizeY) return;
    entry = (appInfoEntry_t *)object->entryList[index];
    flags = entry->flags;
    cursor = &object->cursor;

    //Set the font size
    if (flags & BIG) scaleX = scaleY = 0.6f;
    else if (flags & MEDIUM) scaleX = scaleY = 0.55f;
    else if (flags & SMALL) scaleX = scaleY = 0.45f;
    else if (flags & TINY) scaleX = scaleY = 0.4f;
    
    else scaleX = scaleY = 0.5f;

    //Set the type
    if (flags & BOLD) scaleX += 0.05f;
    else if (flags & SKINNY) scaleY += 0.05f;

    //Set the alignment
    getTextSizeInfos(&textWidth, scaleX, scaleY, entry->buffer);
    if (flags & CENTER)
    {
        temp = object->boundX - cursor->posX;
        temp -= textWidth;
        if (temp > 0)
        {
            temp /= 2;
            cursor->posX += temp;
        }
    }
    if (flags & RIGHT_ALIGN)
    {
        cursor->posX = 297.0f;
        cursor->posX -= textWidth;
    }

    if (flags & NEWLINE)
        cursor->posY += 0.3f * fontGetInfo(NULL)->lineFeed;
    
    //Return the size
    *sizeX = scaleX;
    *sizeY = scaleY;
}
void    drawAppInfoEntry(appInfoObject_t  *object, int index)
{
    float           sizeX;
    float           sizeY;
    float           lineFeed;
    appInfoEntry_t  *entry;
    cursor_t        *cursor;

    if (!object || index >= object->entryCount) goto exit;
    entry = (appInfoEntry_t *)object->entryList[index];
    cursor = &object->cursor;
    sizeX = sizeY = 0.0f;
    getDrawParameters(object, index, &sizeX, &sizeY);
    lineFeed = sizeY * fontGetInfo(NULL)->lineFeed;
    setTextColor(entry->color);
    renderText(cursor->posX, cursor->posY, sizeX, sizeY, false, entry->buffer, cursor);
    cursor->posY += lineFeed;
exit:
    return; 
}

void    drawAppInfo(appInfoObject_t *object)
{
    int         i;
    u32         entryCount;
    float       boundY;
    float       cursorXBak;
    float       cursorYBak;
    cursor_t    *cursor;

    if (!object) return;
    entryCount = object->entryCount;
    if (entryCount <= 0) return;
    boundY = object->boundY;
    cursor = &object->cursor;
    cursorXBak = cursor->posX;
    cursorYBak = cursor->posY;
    setSpritePos(object->sprite, object->spritePosX, object->spritePosY);
    if (showBackground)
        drawSprite(object->sprite);
    for (i = 0; i < entryCount; i++)
    {
        if (cursor->posY >= boundY) break;
        cursor->posX = cursorXBak;
        drawAppInfoEntry(object, i);
    }
    cursor->posX = cursorXBak;
    cursor->posY = cursorYBak;
}