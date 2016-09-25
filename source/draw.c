#include "draw.h"

static DVLB_s           *vshader_dvlb;
static shaderProgram_s  program;
static int              uLoc_projection;
static C3D_Tex          *glyphSheets;
static textVertex_s     *textVtxArray;
static int              textVtxArrayPos;
static drawTarget_t     top;
static drawTarget_t     bottom;
static bool             frameStarted = false;
static gfxScreen_t      currentScreen = -1;
static cursor_t         cursor[2] = { { 10, 10 },{ 10, 10 } };

#define TEXT_VTX_ARRAY_COUNT (4 * 1024)

#define TEX_MIN_SIZE 32

//Grabbed from: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
unsigned int nextPow2(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return (v >= TEX_MIN_SIZE ? v : TEX_MIN_SIZE);
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

void printVertex(textVertex_s *vtx)
{
	printf("Vtx: pos[0] %f, pos[1] %f pos[2] %f, tx[0] %f, tx[1] %f\n",
		vtx->position[0],
		vtx->position[1],
		vtx->position[2],
		vtx->texcoord[0],
		vtx->texcoord[1]
		);
}

static void bindTexture(C3D_Tex *texture)
{
	C3D_TexEnv	*env;

	C3D_TexBind(0, texture);
	env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
}

void setSpritePos(sprite_t *sprite, float posX, float posY)
{
    if (!sprite) return;
    sprite->posX = posX;
    sprite->posY = posY;
}

void drawSprite(sprite_t *sprite)
{
	float       height;
	float       width;
	float       u;
	float       v;
    float       x;
    float       y;
	int         arrayIndex;
	C3D_Tex     *texture;

	if (!sprite) return;
	texture = &sprite->texture;
	height = sprite->height;
	width = sprite->width;
    x = sprite->posX;
    y = sprite->posY;
	u = width / (float)texture->width;
	v = height / (float)texture->height;

	//Bind the sprite's texture
	bindTexture(texture);

	C3D_BufInfo	*bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, textVtxArray, sizeof(textVertex_s), 2, 0x10);

	//Set the vertices
	arrayIndex = textVtxArrayPos;
    addTextVertex(x, y + height, 0.0f, v); //left bottom
    addTextVertex(x + width, y + height, u, v); //right bottom
	addTextVertex(x, y, 0.0f, 0.0f); //left top
	addTextVertex(x + width, y, u, 0.0f); //right top	

	//Draw 
	C3D_DrawArrays(GPU_TRIANGLE_STRIP, arrayIndex, 4);
}

sprite_t *newSprite(int width, int height)
{
    sprite_t    *sprite;
    C3D_Tex     *texture;
    bool        result;

    //Alloc the sprite
    sprite = (sprite_t *)calloc(1, sizeof(sprite_t));
    if (!sprite) goto allocError;
    texture = &sprite->texture;

    //Create and init the sprite's texture
    result = C3D_TexInit(texture, nextPow2(width), nextPow2(height), GPU_RGBA8);
    if (!result) goto texInitError;
    C3D_TexSetWrap(texture, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);

    sprite->width = (float)width;
    sprite->height = (float)height;
    return (sprite);
texInitError:
    free(sprite);
allocError:
    return (NULL);
}

void deleteSprite(sprite_t *sprite)
{
    if (!sprite) return;
    C3D_TexDelete(&sprite->texture);
    free(sprite);
    sprite = NULL;
}

static void sceneInit(void)
{
	int	            i;
	TGLP_s          *glyphInfo;
	C3D_Tex         *tex;
	C3D_AttrInfo    *attrInfo;

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
	Mtx_OrthoTilt(&top.projection, 0.0f, 400.0f, 240.0f, 0.0f, 0.0f, 1.0f, true);
	Mtx_OrthoTilt(&bottom.projection, 0.0f, 320.0f, 240.0f, 0.0f, 0.0f, 1.0f, true);

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

void drawInit(void)
{
    C3D_RenderTarget *target;

    //Init Citro3D
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    // Initialize the top render target
    target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetClear(target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    top.target = target;

    // Initialize the bottom render target
    target = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetClear(target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
    C3D_RenderTargetSetOutput(target, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    bottom.target = target;

    //Initialize the system font
    fontEnsureMapped();

    // Initialize the scene
    sceneInit();
}

void drawEndFrame(void)
{
    if (frameStarted)
    {
        C3D_FrameEnd(0);
        frameStarted = false;
    }
}

void drawExit(void)
{
    drawEndFrame();
    sceneExit();
    C3D_Fini();
}

void setTextColor(u32 color)
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

void getTextSizeInfos(float *width, float scaleX, float scaleY, const char *text)
{
    float   w;
    u8      *c;
    u32     code;
    ssize_t units;
    int     glyphIndex;
    fontGlyphPos_s  data;

    w = 0.0f;
    c = (u8 *)text;
    if (!text) return;
    while (*c == '\n') c++;
    do
    {
        if (!*c) break;
        units = decode_utf8(&code, c);
        if (units == -1) break;
        c += units;
        if (code > 0)
        {
            glyphIndex = fontGlyphIndexFromCodePoint(code);
            fontCalcGlyphPos(&data, glyphIndex, GLYPH_POS_CALC_VTXCOORD, scaleX, scaleY);
            w += data.xAdvance;
        }
    } while (code > 0);
    *width = w;
}

void    findBestSize(float *sizeX, float *sizeY, float posXMin, float posXMax, float sizeMax, const char *text)
{
    float scale;
    float originalTextWidth;
    float margin; //in pixels
    float textWidth;
    float bounds;

    if (!text | !sizeX) return;
    getTextSizeInfos(&originalTextWidth, 1.0f, 1.0f, text);
    scale = sizeMax;
    margin = 1.0f;
    bounds = posXMax - posXMin;
    bounds -= (margin * 2);
    while (1)
    {
        textWidth = scale * originalTextWidth;
        if (textWidth > bounds) scale -= 0.01f;
        else break;
        if (scale <= 0.0f) break;
    }
    *sizeX = scale;
    if (sizeY) *sizeY = scale;
}

void renderText(float x, float y, float scaleX, float scaleY, bool baseline, const char *text, cursor_t *cursor)
{
	u32             flags;
	u32             code;
	int             lastSheet;
	int             glyphIdx;
	int             arrayIndex;
	ssize_t         units;
	float           firstX;
	C3D_BufInfo     *bufInfo;
	fontGlyphPos_s  data;
	const u8        *p = (const u8 *)text;

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
	char        buf[4096];
	va_list     vaList;
	float       posX;
	float       posY;
	
	if (!frameStarted) return;
	posX = POS_X(pos);
	posY = POS_Y(pos);
	va_start(vaList, text);
	setTextColor(color);
	vsnprintf(buf, 4096, text, vaList);
	renderText(posX, posY, size, size, false, buf, NULL);
	va_end(vaList);

}

void updateScreen(void)
{
	if (frameStarted)
		C3D_FrameEnd(0);
	else
		frameStarted = true;
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	textVtxArrayPos = 0;
	cursor[0] = (cursor_t){ 10, 10 };
	cursor[1] = (cursor_t){ 10, 10 };
	currentScreen = -1;
}

void setScreen(gfxScreen_t screen)
{
	if (screen == currentScreen) return;
	currentScreen = screen;
    if (!frameStarted)
    {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        frameStarted = true;
    }
	if (screen == GFX_TOP)
	{
		C3D_FrameDrawOn(top.target);
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &top.projection);
	}
	else if (screen == GFX_BOTTOM)
	{
		C3D_FrameDrawOn(bottom.target);
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &bottom.projection);
	}
	else return;
}
/*
void Printf(u32 color, u32 flags, char *text, ...)
{
	//TODO: Find the best size for BOLD and SKINNY
	char        buf[4096];
	va_list     vaList;
	float       posX;
	float       posY;
	float       sizeX;
	float       sizeY;

	if (flags)
	{
		//Set the font size
		if (flags & BIG) sizeX = sizeY = 1.0f;
		else if (flags & SMALL) sizeX = sizeY = 0.35f;
		else sizeX = sizeY = 0.5f;
		//Set a style
		if (flags & BOLD)
		{
			sizeX = 0.75f;
			sizeY = 0.5f;
		}
		else if (flags & SKINNY)
		{
			sizeX = 0.5f;
			sizeY = 0.75f;
		}
	}
	else
		sizeX = sizeY = 0.5f;
	va_start(vaList, text);
	vsnprintf(buf, 4096, text, vaList);
	posX = cursor[currentScreen].posX;
	posY = cursor[currentScreen].posY;
	setTextColor(color);
	renderText(posX, posY, sizeX, sizeY, false, buf, &cursor[currentScreen]);
	va_end(vaList);
}*/
