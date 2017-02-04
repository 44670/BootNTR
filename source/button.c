#include "button.h"

button_t    *newButton(float posX, float posY, void (*action)(u32), u32 arg, sprite_t *button, sprite_t *text)
{
    button_t    *retButton;

    if (!button || !text) goto error;
    retButton = (button_t *)calloc(1, sizeof(button_t));
    retButton->posX = posX;
    retButton->posY = posY;
    retButton->height = button->height;
    retButton->width = button->width;
    retButton->visible = false;
    retButton->execute = true;
    retButton->buttonSprite = button;
    retButton->textSprite = text;
    retButton->arg = arg;
    retButton->action = action;
    retButton->hide = hideButton;
    retButton->show = showButton;
    retButton->enable = enableButton;
    retButton->disable = disableButton;
    retButton->draw = drawAndExecuteButton;
    retButton->isTouched = buttonIsTouched;
    retButton->destroy = destroyButton;
    return (retButton);
error:
    return (NULL);
}

void        hideButton(button_t *button)
{
    if (!button) goto error;
    button->visible = false;
error:
    return;
}

void        showButton(button_t *button)
{
    if (!button) goto error;
    button->visible = true;
error:
    return;
}

void        enableButton(button_t *button)
{
    if (!button) goto error;
    button->execute = true;
error:
    return;
}

void        disableButton(button_t *button)
{
    if (!button) goto error;
    button->execute = false;
error:
    return;
}

bool        executeButton(button_t *button)
{
    if (!button || !button->execute) goto error;
    if (button->action)
        button->action(button->arg);
    return (true);
error:
    return (false);
}

void        drawButton(button_t *button)
{
    float   posX;
    float   posY;
    float   marginX;
    float   marginY;

    if (!button || !button->visible) goto error;
    posX = button->posX;
    posY = button->posY;
    setSpritePos(button->buttonSprite, posX, posY);
    drawSprite(button->buttonSprite);
    marginX = button->buttonSprite->width - button->textSprite->width;
    if (marginX > 1) marginX = (float)((u32)marginX / 2);
    posX += marginX;
    marginY = button->buttonSprite->height - button->textSprite->height;
    if (marginY > 1) marginY = (float)((u32)marginY / 2);
    posY += marginY;
    setSpritePos(button->textSprite, posX, posY);
    drawSprite(button->textSprite);
error:
    return;
}

bool    drawAndExecuteButton(void *button)
{
    drawButton((button_t *)button);
    if (buttonIsTouched((button_t *)button) && executeButton((button_t *)button))
        return (true);
    return (false);
}

bool    buttonIsTouched(button_t *button)
{
    touchPosition  touchPos;

    if (!button || !button->visible) goto error;
    hidTouchRead(&touchPos);
    if (touchPos.px >= button->posX && touchPos.px <= button->posX + button->width)
    {
        if (touchPos.py >= button->posY && touchPos.py <= button->posY + button->height)
            return (true);
    }
error:
    return (false);
}

void    destroyButton(button_t *button)
{
    if (!button) goto error;
    if (button->textSprite)
        deleteSprite(button->textSprite);
    free(button);
error:
    return;
}

void    moveButton(button_t *button, float posX, float posY)
{
    if (!button) goto error;
    button->posX = posX;
    button->posY = posY;
error:
    return;
}