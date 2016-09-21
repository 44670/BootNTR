#include "draw.h"

static DVLB_s			*vshader_dvlb;
static shaderProgram_s	program;
static int				uLoc_projection;
static C3D_Mtx			projection;
static C3D_Tex			*glyphSheets;
static textVertex_s		*textVtxArray;
static int				textVtxArrayPos;
static C3D_RenderTarget	*topTarget;
static C3D_RenderTarget	*bottomTarget;
static bool				frameStarted = false;
static gfxScreen_t		currentScreen = -1;
static gfxScreen_t		drawOn = 0;
static cursor_t	cursor[2] = { { 10, 10 },{ 10, 10 } };

#define TEXT_VTX_ARRAY_COUNT (4 * 1024)

static void sceneInit(void)
{
	int				i;
	TGLP_s			*glyphInfo;
	C3D_Tex			*tex;
	C3D_AttrInfo	*attrInfo;

	// Load the vertex shader, create a shader program and bind it
	vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
	C3D_BindProgram(&program);

	// Get the location of the uniforms
	uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");

	// Configure attributes for use with the vertex shader
	attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord

												   // Compute the projection matrix
	Mtx_OrthoTilt(&projection, 0.0, 400.0, 240.0, 0.0, 0.0, 1.0, true);

	// Configure depth test to overwrite pixels with the same depth (needed to draw overlapping glyphs)
	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

	// Load the glyph texture sheets
	glyphInfo = fontGetGlyphInfo();
	glyphSheets = malloc(sizeof(C3D_Tex) * glyphInfo->nSheets);
	for (i = 0; i < glyphInfo->nSheets; i++)
	{
		tex = &glyphSheets[i];
		tex->data = fontGetGlyphSheetTex(i);
		tex->fmt = glyphInfo->sheetFmt;
		tex->size = glyphInfo->sheetSize;
		tex->width = glyphInfo->sheetWidth;
		tex->height = glyphInfo->sheetHeight;
		tex->param = GPU_TEXTURE_MAG_FILTER(GPU_LINEAR) | GPU_TEXTURE_MIN_FILTER(GPU_LINEAR)
			| GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_EDGE) | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_EDGE);
	}
	// Create the text vertex array
	textVtxArray = (textVertex_s*)linearAlloc(sizeof(textVertex_s)*TEXT_VTX_ARRAY_COUNT);
}

static void sceneExit(void)
{
	// Free the textures
	free(glyphSheets);

	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
}

static void setTextColor(u32 color)
{
	C3D_TexEnv	*env;
	
	env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, 0, 0);
	C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
	C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
	C3D_TexEnvColor(env, color);
}

static void addTextVertex(float vx, float vy, float tx, float ty)
{
	textVertex_s	*vtx;
	
	vtx = &textVtxArray[textVtxArrayPos++];
	vtx->position[0] = vx;
	vtx->position[1] = vy;
	vtx->position[2] = 0.5f;
	vtx->texcoord[0] = tx;
	vtx->texcoord[1] = ty;
}

static void renderText(float x, float y, float scaleX, float scaleY, bool baseline, const char *text, cursor_t *cursor)
{
	u32				flags;
	u32				code;
	int				lastSheet;
	int				glyphIdx;
	int				arrayIndex;
	ssize_t			units;
	float			firstX;
	C3D_BufInfo		*bufInfo;
	fontGlyphPos_s	data;
	const u8		*p = (const u8 *)text;

	// Configure buffers
	bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, textVtxArray, sizeof(textVertex_s), 2, 0x10);	
	firstX = x;
	flags = GLYPH_POS_CALC_VTXCOORD | (baseline ? GLYPH_POS_AT_BASELINE : 0);
	lastSheet = -1;
	do
	{
		if (!*p)
			break;
		units = decode_utf8(&code, p);
		if (units == -1)
			break;
		p += units;
		if (code == '\n')
		{
			x = firstX;
			y += scaleY * fontGetInfo()->lineFeed;
		}
		else if (code > 0)
		{
			glyphIdx = fontGlyphIndexFromCodePoint(code);
			fontCalcGlyphPos(&data, glyphIdx, flags, scaleX, scaleY);

			// Bind the correct texture sheet
			if (data.sheetIndex != lastSheet)
			{
				lastSheet = data.sheetIndex;
				C3D_TexBind(0, &glyphSheets[lastSheet]);
			}

			arrayIndex = textVtxArrayPos;
			if ((arrayIndex + 4) >= TEXT_VTX_ARRAY_COUNT)
				break; // We can't render more characters

					   // Add the vertices to the array
			addTextVertex(x + data.vtxcoord.left, y + data.vtxcoord.bottom, data.texcoord.left, data.texcoord.bottom);
			addTextVertex(x + data.vtxcoord.right, y + data.vtxcoord.bottom, data.texcoord.right, data.texcoord.bottom);
			addTextVertex(x + data.vtxcoord.left, y + data.vtxcoord.top, data.texcoord.left, data.texcoord.top);
			addTextVertex(x + data.vtxcoord.right, y + data.vtxcoord.top, data.texcoord.right, data.texcoord.top);

			// Draw the glyph
			C3D_DrawArrays(GPU_TRIANGLE_STRIP, arrayIndex, 4);

			x += data.xAdvance;

		}
	} while (code > 0);
	if (cursor)
	{
		cursor->posX = x;
		cursor->posY = y;
	}
}

void drawText(screenPos_t pos, float size, u32 color, char *text, ...)
{
	char		buf[4096];
	va_list		vaList;
	float		posX;
	float		posY;
	
	if (!frameStarted) return;
	posX = POS_X(pos);
	posY = POS_Y(pos);
	va_start(vaList, text);
	setTextColor(color);
	vsnprintf(buf, 4096, text, vaList);
	renderText(posX, posY, size, size, false, buf, NULL);
	va_end(vaList);

}

void drawInit(void)
{
	Result				result;

	//Init Citro3D
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	// Initialize the render top target
	topTarget = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetClear(topTarget, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(topTarget, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	// Initialize the render bottom target
	bottomTarget = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetClear(bottomTarget, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(bottomTarget, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	//Initialize the system font
	result = fontEnsureMapped();

	// Initialize the scene
	sceneInit();

	//Start frame
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	frameStarted = true;
}

void drawExit(void)
{
	if (frameStarted)
	{
		C3D_FrameEnd(0);
		frameStarted = false;
	}
	sceneExit();
	C3D_Fini();
}

void updateScreen(void)
{
	if (frameStarted)
		C3D_FrameEnd(0);
	else
		frameStarted = true;
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
	textVtxArrayPos = 0;
	cursor[0] = (cursor_t){ 10, 10 };
	cursor[1] = (cursor_t){ 10, 10 };
}

void setScreen(gfxScreen_t screen)
{
	currentScreen = screen;
	if (screen == GFX_TOP) C3D_FrameDrawOn(topTarget);
	else if (screen == GFX_BOTTOM) C3D_FrameDrawOn(bottomTarget);
	else return;
	drawOn = screen;
}

void Printf(u32 color, u32 flags, char *text, ...)
{
	char		buf[4096];
	va_list		vaList;
	float		posX;
	float		posY;
	float		sizeX;
	float		sizeY;

	if (flags)
	{
		//Set the font size
		if (flags & BIG) sizeX = sizeY = 1.0f;
		else if (flags & SMALL) sizeX = sizeY = 0.5f;
		else sizeX = sizeY = 0.65f;
		//Set a style
		if (flags & BOLD)
		{
			sizeX = 0.75f;
			sizeY = 0.65f;
		}
		else if (flags & SKINNY)
		{
			sizeX = 0.55f;
			sizeY = 0.65f;
		}
		//Cursor
		if (flags & NEWLINE) cursor[currentScreen].posY += sizeY * fontGetInfo()->lineFeed;
	}
	else
		sizeX = sizeY = 0.65f;
	va_start(vaList, text);
	vsnprintf(buf, 4096, text, vaList);
	posX = cursor[currentScreen].posX;
	posY = cursor[currentScreen].posY;
	setTextColor(color);
	renderText(posX, posY, sizeX, sizeY, false, buf, &cursor[currentScreen]);
	//cursor[currentScreen].posX = POS_X(screenPos);
	//cursor[currentScreen].posY = POS_Y(screenPos);
	va_end(vaList);
}
