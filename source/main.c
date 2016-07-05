#include "main.h"

NTR_CONFIG		g_ntrConfig = { 0 };
BOOTNTR_CONFIG	g_bnConfig = { 0 };
NTR_CONFIG		*ntrConfig;
BOOTNTR_CONFIG	*bnConfig;
u8				*tmpBuffer;
char			*g_primary_error = NULL;
char			*g_secondary_error = NULL;
char			*g_third_error = NULL;
bool			g_exit = false;

#define SELECTOR

int main(void)
{
	u32		keys;

	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);
#ifdef SELECTOR
	printf("\nBootNTR Selector by Nanquitas\n\n");
	printf("Hello !\n\n");
	printf("Which version of NTR do I load ?\n");
	printf(" - 3.2  : Press X\n");
	printf(" - 3.3  : Press A\n");
	printf(" - 3.4  : Press Y\n");
	printf(" - Exit : Press B\n\n");
	while (1)
	{
		hidScanInput();
		keys = (hidKeysDown() | hidKeysUp() | hidKeysHeld());
		if (keys & KEY_X)
		{
			printf("Loading NTR 3.2 ...\n\n");
			remove("sdmc:/ntr.bin");
			check_prim(copy_file("sdmc:/ntr_3_2.bin", "sdmc:/ntr.bin"), FILE_COPY_ERROR);
			break;
		}
		else if (keys & KEY_A)
		{
			printf("Loading NTR 3.3 ...\n\n");
			remove("sdmc:/ntr.bin");
			check_prim(copy_file("sdmc:/ntr_3_3.bin", "sdmc:/ntr.bin"), FILE_COPY_ERROR);
			break;
		}
		else if (keys & KEY_Y)
		{
			printf("Loading NTR 3.4 ...\n\n");
			remove("sdmc:/ntr.bin");
			check_prim(copy_file("sdmc:/ntr_3_4.bin", "sdmc:/ntr.bin"), FILE_COPY_ERROR);
			break;
		}
		if (abort_and_exit())
			goto error;
	}
#else
	printf("\nBootNTR by Nanquitas\n\n");
	printf("Hello !\n\n");
	printf("Loading the ntr.bin you provided...\n\n");
#endif
	memset(&g_ntrConfig, 0, sizeof(g_ntrConfig));
	memset(&g_bnConfig, 0, sizeof(g_bnConfig));
	ntrConfig = &g_ntrConfig;
	bnConfig = &g_bnConfig;
	abort_and_exit();
#ifdef SELECTOR
	error:
#endif
	if (!g_exit && bnBootNTR() == 0)
	{
		printf("NTR CFW loaded successfully !\n");
		svcSleepThread(1000000000);
	}
	else
	{

		printf("The loading of NTR failed.\n");
		if (g_primary_error != NULL)
			printf("#%s\n", g_primary_error);
		if (g_secondary_error != NULL)
			printf("#%s\n", g_secondary_error);
		if (g_third_error != NULL)
			printf("#%s\n", g_third_error);
		printf("\nYou should reboot your console and try again.\n\n");
		printf("Press any key to exit.");
		while (aptMainLoop())
		{
			hidScanInput();
			keys = hidKeysDown();
			if (keys)
				break;
		}
	}
	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForVBlank();
	gfxExit();
	return (0);
}
