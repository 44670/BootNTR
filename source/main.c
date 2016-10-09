#include "main.h"
#include "draw.h"
#include "config.h"
#include "button.h"
#include <time.h>

extern bootNtrConfig_t *bnConfig;
u8				  *tmpBuffer;
char			  *g_primary_error = NULL;
char			  *g_secondary_error = NULL;
char			  *g_third_error = NULL;
bool			  g_exit = false;


int main(void)
{
    u32         keys;
    u32         kernelVersion;
    int         ret;

	gfxInitDefault();
	drawInit();
	romfsInit();
    initUI();
    hidScanInput();
    keys = (hidKeysDown() | hidKeysHeld());
    if (keys & KEY_SELECT)
        resetConfig();
    configInit();
    kernelVersion = osGetKernelVersion();
    initMainMenu();
    ret = mainMenu();

    if (ret == 2) goto waitForExit;
    if (!g_exit && bnBootNTR() == 0)
    {
        newAppStatus(DEFAULT_COLOR, CENTER | BOLD | NEWLINE, "Success !");
        newAppStatus(DEFAULT_COLOR, CENTER | TINY | SKINNY | NEWLINE, "Returning to home");
        newAppStatus(DEFAULT_COLOR, CENTER | TINY | SKINNY, "menu ...");
        updateUI();
        svcSleepThread(100000);
    }
	else
	{
        newAppStatus(DEFAULT_COLOR, CENTER | BOLD | NEWLINE, "Load failed !");
        if (!g_third_error) newAppStatus(DEFAULT_COLOR, CENTER | BOLD, "\uE00A");
        if (g_primary_error != NULL)
        {
            if (g_primary_error == UNKNOWN_FIRM)
            {
                newAppStatus(DEFAULT_COLOR, TINY | CENTER, "#Firmware unknown");
                newAppStatus(DEFAULT_COLOR, TINY | CENTER, "#Detected firm: %d.%d.%d", \
                    GET_VERSION_MAJOR(kernelVersion), \
                    GET_VERSION_MINOR(kernelVersion), \
                    GET_VERSION_REVISION(kernelVersion));
            }
            else
                newAppStatus(DEFAULT_COLOR, TINY | CENTER, "#%s", g_primary_error);
        }            
        if (g_secondary_error != NULL)
            newAppStatus(DEFAULT_COLOR, TINY | CENTER, "#%s", g_secondary_error);
        if (g_third_error != NULL)
            newAppStatus(DEFAULT_COLOR, TINY | CENTER, "#%s", g_third_error);
waitForExit:
        newAppStatus(DEFAULT_COLOR, CENTER | TINY | SKINNY | NEWLINE, "Press any key to");
        newAppStatus(DEFAULT_COLOR, CENTER | TINY | SKINNY, "return to home menu");
        while (aptMainLoop())
        {
            updateUI();
            hidScanInput();
            keys = hidKeysDown();
            if (keys)
                break;
        }
	}
    configExit();
    exitMainMenu();
    exitUI();
	romfsExit();
	drawExit();
	gfxExit();
	return (0);
}
