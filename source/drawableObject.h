#ifndef DRAWABLEOBJECT_H
#define DRAWABLEOBJECT_H
#define MAX_ELEMENTS 15

#include "draw.h"

typedef struct  drawableObject_s
{
    bool        (*draw)(void *self);
}               drawableObject_t;

typedef struct  text_s
{
    /* herited from drawableObject_t */
    bool        (*draw)(void *self);

    char        *str;
    int         color;
    float       posX;
    float       posY;
    float       scaleX;
    float       scaleY;
    bool        visible;
}               text_t;

typedef struct  image_s
{
    /* herited from drawableObject_t */
    bool        (*draw)(void *self);

    sprite_t    *sprite;
}               image_t;

typedef struct  window_s
{
    /* herited from drawableObject_t */
    bool        (*draw)(void *self);

    sprite_t    *title;
    sprite_t    *content;
    sprite_t    *background;
}               window_t;

typedef struct   backgroundScreen_s
{
    /* herited from drawableObject_t */
    bool        (*draw)(void *self);

    sprite_t     *background;
    sprite_t     *headerText;
    sprite_t     *footerText;

}                backgroundScreen_t;

typedef struct  drawableScreen_s
{
    /* herited from drawableObject_t */
    bool        (*draw)(void *self);

    backgroundScreen_t  *background;
    int                 elementsCount;
    int                 elementList[MAX_ELEMENTS];
}               drawableScreen_t;

backgroundScreen_t  *newBackgroundObject(sprite_t *background, \
    sprite_t *header, sprite_t *footer);
bool        drawBackground(void *self);
void        changeBackgroundHeader(backgroundScreen_t *bg, sprite_t *header);
void        changeBackgroundFooter(backgroundScreen_t *bg, sprite_t *footer);

drawableScreen_t    *newDrawableScreen(backgroundScreen_t *background);
bool        drawScreen(void *self);
void        addObjectToScreen(drawableScreen_t *screen, void *object);
void        *removeLastObjectFromScreen(drawableScreen_t *screen);
void        clearObjectListFromScreen(drawableScreen_t *screen);

image_t     *newImage(sprite_t *sprite);
bool        drawImage(void *self);

window_t    *newWindow(sprite_t *background, sprite_t *title, \
    sprite_t *content);
bool       drawWindow(void *self);
void    changeWindowContent(window_t *window, sprite_t *content);
void    changeWindowTitle(window_t *window, sprite_t *title);

text_t      *newText(char *str, float posX, float posY, float scaleX, float scaleY, int color);
bool    drawTextObject(void *self);
void        hideText(text_t *text);
void        showText(text_t *text);
#endif
