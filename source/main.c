#include "main.h"
#include "draw.h"

NTR_CONFIG		g_ntrConfig = { 0 };
BOOTNTR_CONFIG	g_bnConfig = { 0 };
NTR_CONFIG		*ntrConfig;
BOOTNTR_CONFIG	*bnConfig;
u8				*tmpBuffer;
char			*g_primary_error = NULL;
char			*g_secondary_error = NULL;
char			*g_third_error = NULL;
bool			g_exit = false;
sprite_t		*bottomSprite;
sprite_t		*topSprite;
sprite_t		*infoSprite;
bool            displayInfo = false;

void printMenu(int update)
{
	setScreen(GFX_TOP);
    drawSprite(0, 0, topSprite);
	setScreen(GFX_BOTTOM);
    drawSprite(0, 0, bottomSprite);
    if (displayInfo)
        drawSprite(0, 0, infoSprite);
	if (update)
		updateScreen();
}

int main(void)
{
	u32		keys;
    touchPosition touchPos;

	gfxInitDefault();
    //gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, true);
	//consoleInit(GFX_BOTTOM, NULL);
	drawInit();
	romfsInit();
    loadPNGFile(&topSprite, "romfs:/TOP.png");
    loadPNGFile(&bottomSprite, "romfs:/BOT.png");
    loadPNGFile(&infoSprite, "romfs:/INFO.png");
	while (1)
	{
		hidScanInput();
        hidTouchRead(&touchPos);
		keys = (hidKeysDown());
        if (touchPos.px >= 11 && touchPos.px <= 99)
        {
            if (touchPos.py >= 42 && touchPos.py <= 92)
            {
                printMenu(0);
                Printf(COLOR_BLANK, BOLD, "Loading NTR 3.2 ...\n\n");
                updateScreen();
                remove("sdmc:/ntr.bin");
                check_prim(copy_file("sdmc:/ntr_3_2.bin", "sdmc:/ntr.bin"), FILE_COPY_ERROR);
                break;
            }
            else if (touchPos.py >= 99 && touchPos.py <= 150)
            {
                printMenu(0);
                Printf(COLOR_BLANK, BOLD, "Loading NTR 3.3 ...\n\n");
                updateScreen();
                remove("sdmc:/ntr.bin");
                check_prim(copy_file("sdmc:/ntr_3_3.bin", "sdmc:/ntr.bin"), FILE_COPY_ERROR);
                break;
            }
            else if (touchPos.py >= 156 && touchPos.py <= 208)
            {
                printMenu(0);
                Printf(COLOR_BLANK, BOLD, "Loading NTR 3.4 ...\n\n");
                updateScreen();
                remove("sdmc:/ntr.bin");
                check_prim(copy_file("sdmc:/ntr_3_4.bin", "sdmc:/ntr.bin"), FILE_COPY_ERROR);
                break;
            }
        }
        if (keys & KEY_A)
            displayInfo = !displayInfo;
		if (abort_and_exit())
			goto error;
		printMenu(1);
	}
	memset(&g_ntrConfig, 0, sizeof(g_ntrConfig));
	memset(&g_bnConfig, 0, sizeof(g_bnConfig));
	ntrConfig = &g_ntrConfig;
	bnConfig = &g_bnConfig;
	abort_and_exit();
error:	
	if (!g_exit && bnBootNTR() == 0)
	{
		setScreen(GFX_BOTTOM);
		Printf(COLOR_DARKGREEN, 0, "NTR CFW loaded successfully !\n");
		updateScreen();
		svcSleepThread(1000000000);
	}
	else
	{
		while (aptMainLoop())
		{
			setScreen(GFX_BOTTOM);
			Printf(COLOR_RED, BOLD, "The loading of NTR failed.\n");
			if (g_primary_error != NULL)
				Printf(COLOR_BLACK, SMALL, "#%s\n", g_primary_error);
			if (g_secondary_error != NULL)
				Printf(COLOR_BLACK, SMALL, "#%s\n", g_secondary_error);
			if (g_third_error != NULL)
				Printf(COLOR_BLACK, SMALL, "#%s\n", g_third_error);
			Printf(COLOR_BLANK, 0, "\nYou should reboot your console and try again.\n\n");
			Printf(COLOR_BLANK, BOLD, "Press any key to exit.");
			updateScreen();
			hidScanInput();
			keys = hidKeysDown();
			if (keys)
				break;
		}
	}
	romfsExit();
	drawExit();
	gfxExit();
	return (0);
}
