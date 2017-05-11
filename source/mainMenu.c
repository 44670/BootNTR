#include "main.h"
#include "draw.h"
#include "config.h"
#include "button.h"
#include <time.h>

extern bootNtrConfig_t  *bnConfig;

static button_t         *V32Button;
static button_t         *V33Button;
static button_t         *V35Button;
static sprite_t         *desiredVersionSprite;
static sprite_t         *tinyButtonBGSprite;
static sprite_t         *pressExitSprite;
static bool             userTouch = false;

void    selectVersion(u32 mode)
{
    V32Button->disable(V32Button);
    V33Button->disable(V33Button);
    V35Button->disable(V35Button);
    userTouch = true;
    switch(mode)
    {
        case 1:
            bnConfig->versionToLaunch = V32;
            break;
        case 2:
            bnConfig->versionToLaunch = V33;
            break;
        case 3:
            bnConfig->versionToLaunch = V35;
            break;
        default:
            break;
    }
}

void    initMainMenu(void)
{
    if (!bnConfig->isMode3)
    {
        sprite_t *sprite;

        newSpriteFromPNG(&desiredVersionSprite, "romfs:/sprites/textSprites/touchDesiredVersion.png");    
        newSpriteFromPNG(&tinyButtonBGSprite, "romfs:/sprites/tinyButtonBackground.png");

        setSpritePos(desiredVersionSprite, 34.0f, 7.0f);

        changeBottomHeader(desiredVersionSprite);

        newSpriteFromPNG(&sprite, "romfs:/sprites/textSprites/32Version.png");
        V32Button = newButton(11.0f, 35.0f, selectVersion, 1, tinyButtonBGSprite, sprite);
        newSpriteFromPNG(&sprite, "romfs:/sprites/textSprites/33Version.png");
        V33Button = newButton(11.0f, 94.0f, selectVersion, 2, tinyButtonBGSprite, sprite);
        newSpriteFromPNG(&sprite, "romfs:/sprites/textSprites/35Version.png");
        V35Button = newButton(11.0f, 152.0f, selectVersion, 3, tinyButtonBGSprite, sprite);

        V32Button->show(V32Button);
        V33Button->show(V33Button);
        V35Button->show(V35Button);
        addBottomObject(V32Button);
        addBottomObject(V33Button);
        addBottomObject(V35Button);        
    }

    newSpriteFromPNG(&pressExitSprite, "romfs:/sprites/textSprites/pressBExit.png");

    setSpritePos(pressExitSprite, 180.0f, 217.0f);

    changeBottomFooter(pressExitSprite); 

}

void    exitMainMenu(void)
{
    if (!bnConfig->isMode3)
    {
        destroyButton(V32Button);
        destroyButton(V33Button);
        destroyButton(V35Button);
        deleteSprite(tinyButtonBGSprite);
        deleteSprite(desiredVersionSprite);
    }

    deleteSprite(pressExitSprite);
}


static const char * versionString[] =
{
    "3.2",
    "3.3",
    "3.5"
};

int     mainMenu(void)
{
    time_t      baseTime;
    int         timer;
    int         timerBak;
    u32         keys;
    bool        noTimer;

    waitAllKeysReleased();
    if (!bnConfig->isMode3 && !bnConfig->config->flags) noTimer = true;
    else noTimer = false;
    appInfoDisableAutoUpdate();
    if (!noTimer)
    {
        timerBak = timer = TIMER;
        newAppStatus(DEFAULT_COLOR, CENTER | TINY | SKINNY, "Loading %s in %d", versionString[bnConfig->versionToLaunch], timerBak);
        baseTime = time(NULL);
        updateUI();
    }    
    keys = 0;

    while (userTouch == false)
    {
        keys = hidKeysDown() | hidKeysHeld();
        if (keys == (KEY_L | KEY_R | KEY_X | KEY_DUP)) goto dumpMode;
        if (keys && !bnConfig->isMode3)
        {
            noTimer = true;
            removeAppStatus();
            updateUI();
        }
        if (abort_and_exit()) goto abort;
        if (!noTimer)
        {
            timer -= (time(NULL) - baseTime);
            if (timer != timerBak)
            {
                timerBak = timer;
                removeAppStatus();
                newAppStatus(DEFAULT_COLOR, CENTER | TINY | SKINNY, "Loading %s in %d", versionString[bnConfig->versionToLaunch], timerBak);
                baseTime = time(NULL);
            }
            if (timer <= 0)
                break;
        }        
        updateUI();
    }
    if (!noTimer)
        removeAppStatus();
    appInfoEnableAutoUpdate();
    newAppStatus(DEFAULT_COLOR, CENTER | TINY | SKINNY, "Loading %s ...", versionString[bnConfig->versionToLaunch]);
    return (1);

abort:
    appInfoEnableAutoUpdate();
    return (0);
    
dumpMode:
    removeAppStatus();
    appInfoEnableAutoUpdate();
    ntrDumpMode();
    return (2);

}

void    ntrDumpMode(void)
{
    newAppTop(DEFAULT_COLOR, TINY | SKINNY, "Starting NTR Dump Mode...");
    newAppTop(DEFAULT_COLOR, TINY, "");
    newAppStatus(DEFAULT_COLOR, TINY | SKINNY | CENTER, "NTR Dump Mode");
    newAppStatus(DEFAULT_COLOR, TINY | SKINNY | CENTER, "See the top screen");
    updateUI();
    launchNTRDumpMode();
    newAppTop(DEFAULT_COLOR, TINY | SKINNY, "Done !");
    newAppTop(DEFAULT_COLOR, TINY | SKINNY | NEWLINE, "Exiting Dump Mode...");
    newAppStatus(DEFAULT_COLOR, CENTER | BOLD | NEWLINE, "Done !");
    updateUI();
}