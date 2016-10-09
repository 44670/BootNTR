#ifndef BUTTON_H
#define BUTTON_H

#include <stdlib.h>
#include <3ds/types.h>
#include <3ds/services/hid.h>
#include "draw.h"

typedef struct  button_s
{
    /*herited from drawableObject_t*/
    bool        (*draw)(void *self);

    float       posX;
    float       posY;
    float       width;
    float       height;
    bool        visible;
    bool        execute;
    sprite_t    *buttonSprite;
    sprite_t    *textSprite;
    void        (*action)(u32 arg);
    u32         arg;

    void        (*hide)(struct button_s *self);
    void        (*show)(struct button_s *self);
    void        (*enable)(struct button_s *self);
    void        (*disable)(struct button_s *self);
    bool        (*isTouched)(struct button_s *self);
    void        (*destroy)(struct button_s *self);
}               button_t;

button_t        *newButton(float posX, float posY, void (*action)(u32), u32 arg, sprite_t *button, sprite_t *text);
void            hideButton(button_t *button);
void            showButton(button_t *button);
void            enableButton(button_t *button);
void            disableButton(button_t *button);
bool            executeButton(button_t *button);
void            drawButton(button_t *button);
bool            drawAndExecuteButton(void *button);
bool            buttonIsTouched(button_t *button);
void            moveButton(button_t *button, float posX, float posY);
/* destroy doesn't free buttonSprite, only textSprite */
void            destroyButton(button_t *button);

#endif
