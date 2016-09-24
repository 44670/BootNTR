#include "graphics.h"

static sprite_t		*bottomSprite;
static sprite_t		*topSprite;
static sprite_t		*infoSprite;

void    initUI(void)
{
    loadPNGFile(&topSprite, "romfs:/TOP.png");
    loadPNGFile(&bottomSprite, "romfs:/BOT.png");
    loadPNGFile(&infoSprite, "romfs:/INFO.png");
    setSpritePos(topSprite, 0, 0);
    setSpritePos(bottomSprite, 0, 0);
    setSpritePos(infoSprite, 0, 0);
    appInfoInit(infoSprite);
}

void    exitUI(void)
{
    drawEndFrame();
    deleteSprite(bottomSprite);
    deleteSprite(topSprite);
    deleteSprite(infoSprite);
}

static inline void drawUITop(void)
{
    setScreen(GFX_TOP);
    drawSprite(topSprite);
}

static inline void drawUIBottom(void)
{
    setScreen(GFX_BOTTOM);
    drawSprite(bottomSprite);
    drawAppInfo();
}

void    updateUI(void)
{

    drawUITop();
    drawUIBottom();
    updateScreen();
}