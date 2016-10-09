#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "draw.h"
#include "main.h"
#include "appInfo.h"

#define STACKSIZE 0x1000

void    initUI(void);
void    exitUI(void);
int     updateUI(void);
void    addTopObject(void *object);
void    addBottomObject(void *object);
void    changeTopFooter(sprite_t *footer);
void    changeTopHeader(sprite_t *header);
void    changeBottomFooter(sprite_t *footer);
void    changeBottomHeader(sprite_t *header);
void    clearTopScreen(void);
void    clearBottomScreen(void);

extern appInfoObject_t  *appStatus;
extern appInfoObject_t  *appTop;
extern appInfoObject_t  *appBottom;

#define newAppStatus(...) newAppInfoEntry(appStatus, __VA_ARGS__)
#define newAppTop(...) newAppInfoEntry(appTop, __VA_ARGS__)
#define removeAppStatus() removeAppInfoEntry(appStatus)
#define removeAppTop() removeAppInfoEntry(appTop)
#define clearStatus(update)   clearAppInfo(appStatus, update)
#define clearTop(update)    clearAppInfo(appTop, update)

#endif