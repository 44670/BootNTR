#include "main.h"
#include "draw.h"
#include "graphics.h"

NTR_CONFIG		g_ntrConfig = { 0 };
BOOTNTR_CONFIG	g_bnConfig = { 0 };
NTR_CONFIG		*ntrConfig;
BOOTNTR_CONFIG	*bnConfig;
u8				*tmpBuffer;
char			*g_primary_error = NULL;
char			*g_secondary_error = NULL;
char			*g_third_error = NULL;
bool			g_exit = false;

int main(void)
{
	u32		keys;
    u32		kernelVersion = osGetKernelVersion();
    touchPosition touchPos;

	gfxInitDefault();
    //gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, true);
	//consoleInit(GFX_BOTTOM, NULL);
	drawInit();
	romfsInit();
    initUI();
	while (aptMainLoop())
	{
		hidScanInput();
        hidTouchRead(&touchPos);
		keys = (hidKeysDown());
        if (touchPos.px >= 11 && touchPos.px <= 99)
        {
            if (touchPos.py >= 42 && touchPos.py <= 92)
            {
               // newAppInfoEntry(DEFAULT_COLOR, CENTER, "3.2 Selected");
                newAppInfoEntry(DEFAULT_COLOR, CENTER, "Loading 3.2 ...");
                remove("sdmc:/ntr.bin");
                check_prim(copy_file("sdmc:/ntr_3_2.bin", "sdmc:/ntr.bin"), FILE_COPY_ERROR);
                break;
            }
            else if (touchPos.py >= 99 && touchPos.py <= 150)
            {
              //  newAppInfoEntry(DEFAULT_COLOR, CENTER, "3.3 Selected");
                newAppInfoEntry(DEFAULT_COLOR, CENTER, "Loading 3.3 ...");
                remove("sdmc:/ntr.bin");
                check_prim(copy_file("sdmc:/ntr_3_3.bin", "sdmc:/ntr.bin"), FILE_COPY_ERROR);
                break;
            }
            else if (touchPos.py >= 156 && touchPos.py <= 208)
            {
               // newAppInfoEntry(DEFAULT_COLOR, CENTER, "3.4 Selected");
                newAppInfoEntry(DEFAULT_COLOR, CENTER, "Loading 3.4 ...");
                remove("sdmc:/ntr.bin");
                check_prim(copy_file("sdmc:/ntr_3_4.bin", "sdmc:/ntr.bin"), FILE_COPY_ERROR);
                break;
            }
        }
		if (abort_and_exit())
			goto error;
        updateUI();
      //  svcSleepThread(100);
	}
	memset(&g_ntrConfig, 0, sizeof(g_ntrConfig));
	memset(&g_bnConfig, 0, sizeof(g_bnConfig));
	ntrConfig = &g_ntrConfig;
	bnConfig = &g_bnConfig;
	abort_and_exit();
error:	
	if (!g_exit && bnBootNTR() == 0)
	{
        newAppInfoEntry(DEFAULT_COLOR, CENTER | BIG | NEWLINE, "Success !");
        newAppInfoEntry(DEFAULT_COLOR, CENTER | SMALL | BOLD |NEWLINE, "Returning to home");
        newAppInfoEntry(DEFAULT_COLOR, CENTER |SMALL | BOLD, "menu ...");
        updateUI();
		svcSleepThread(1000000000);
	}
	else
	{
        newAppInfoEntry(DEFAULT_COLOR, CENTER | BOLD | NEWLINE, "Load failed !");
        newAppInfoEntry(DEFAULT_COLOR, CENTER | BOLD, "\uE00A");
        if (g_primary_error != NULL)
        {
            if (g_primary_error == UNKNOWN_FIRM)
            {
                newAppInfoEntry(DEFAULT_COLOR, SMALL | CENTER, "#Firmware unknown");
                newAppInfoEntry(DEFAULT_COLOR, SMALL | CENTER, "#Detected firm: %d.%d.%d", \
                    GET_VERSION_MAJOR(kernelVersion), \
                    GET_VERSION_MINOR(kernelVersion), \
                    GET_VERSION_REVISION(kernelVersion));
            }
            else
                newAppInfoEntry(DEFAULT_COLOR, SMALL | CENTER, "#%s", g_primary_error);
        }            
        if (g_secondary_error != NULL)
            newAppInfoEntry(DEFAULT_COLOR, SMALL | CENTER, "#%s", g_secondary_error);
        if (g_third_error != NULL)
            newAppInfoEntry(DEFAULT_COLOR, SMALL | CENTER, "#%s", g_third_error);
        newAppInfoEntry(DEFAULT_COLOR, CENTER | SMALL | BOLD | NEWLINE, "Press any key to");
        newAppInfoEntry(DEFAULT_COLOR, CENTER | SMALL | BOLD, "return to home menu");
        while (aptMainLoop())
        {
            updateUI();
            hidScanInput();
            keys = hidKeysDown();
            if (keys)
                break;
        }
	}
    exitUI();
	romfsExit();
	drawExit();
	gfxExit();
	return (0);
}
