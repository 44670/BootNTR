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
sprite_t		*bottom_sprite;
sprite_t		*top_sprite;
sprite_t		*info_sprite;

Result			romfsResult;

void printMenu(int update)
{
	//printf("Draw");
	setScreen(GFX_TOP);
	//drawText(SCREEN_POS(115, 70), 1.5f, COLOR_BLUE, "Boot NTR");
	//drawText(SCREEN_POS(150, 110), 1.0f, COLOR_BLANK, "Selector");
	if (top_sprite)
		draw_sprite(0, 0, top_sprite);
	//printf("1");
	setScreen(GFX_BOTTOM);
	Printf(COLOR_BLANK, 0, "Which version should I load ?\n");
	Printf(COLOR_BLANK, 0, " - 3.2  : Press \uE002\n");
	Printf(COLOR_BLANK, 0, " - 3.3  : Press \uE000\n");
	Printf(COLOR_BLANK, 0, " - 3.4  : Press \uE003\n");
	Printf(COLOR_RED, 0, " - Exit : Press \uE001\n\n");
	if (R_SUCCEEDED(romfsResult))
		Printf(COLOR_BLACK, BOLD, "ROMFS Success\n");
	Printf(COLOR_BLACK, BOLD, "Bottom: %08X\n", bottom_sprite);
	Printf(COLOR_BLACK, BOLD, "Top: %08X\n", top_sprite);
	Printf(COLOR_BLACK, BOLD, "Info: %08X\n", info_sprite);
	//printf("2");
	if (update)
		updateScreen();
	//printf("end\n");
}

int main(void)
{
	u32		keys;

	gfxInitDefault();
	//consoleInit(GFX_BOTTOM, NULL);
	drawInit();
	romfsResult = romfsInit();
	//printf("Init done\n");
	//bottom_sprite = load_png("romfs:/BOT.png");
	top_sprite = load_png("romfs:/TOP.png");
	//info_sprite = load_png("romfs:/INFO.png");
	while (1)
	{
		hidScanInput();
		keys = (hidKeysDown() | hidKeysUp() | hidKeysHeld());		
		if (keys & KEY_X)
		{
			printMenu(0);
			Printf(COLOR_BLANK, BOLD, "Loading NTR 3.2 ...\n\n");
			updateScreen();
			remove("sdmc:/ntr.bin");
			check_prim(copy_file("sdmc:/ntr_3_2.bin", "sdmc:/ntr.bin"), FILE_COPY_ERROR);
			break;
		}
		else if (keys & KEY_A)
		{
			printMenu(0);
			Printf(COLOR_BLANK, BOLD, "Loading NTR 3.3 ...\n\n");
			updateScreen();
			remove("sdmc:/ntr.bin");
			check_prim(copy_file("sdmc:/ntr_3_3.bin", "sdmc:/ntr.bin"), FILE_COPY_ERROR);
			break;
		}
		else if (keys & KEY_Y)
		{
			printMenu(0);
			Printf(COLOR_BLANK, BOLD, "Loading NTR 3.4 ...\n\n");
			updateScreen();
			remove("sdmc:/ntr.bin");
			check_prim(copy_file("sdmc:/ntr_3_4.bin", "sdmc:/ntr.bin"), FILE_COPY_ERROR);
			break;
		}
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
