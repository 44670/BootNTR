#include "ctrcommon/gpu.hpp"

#include "service.hpp"

#include <malloc.h>
#include <string.h>

#include <3ds.h>

#define GPU_COMMAND_BUFFER_SIZE 0x80000

#define TEX_ENV_COUNT 6
#define TEX_UNIT_COUNT 3

#define STATE_VIEWPORT (1 << 0)
#define STATE_DEPTH_MAP (1 << 1)
#define STATE_CULL (1 << 2)
#define STATE_STENCIL_TEST (1 << 3)
#define STATE_BLEND (1 << 4)
#define STATE_ALPHA_TEST (1 << 5)
#define STATE_DEPTH_TEST_AND_MASK (1 << 6)
#define STATE_ACTIVE_SHADER (1 << 7)
#define STATE_TEX_ENV (1 << 8)
#define STATE_TEXTURES (1 << 9)
#define STATE_SCISSOR_TEST (1 << 10)

typedef struct {
    DVLB_s* dvlb;
    shaderProgram_s program;
} ShaderData;

typedef struct {
    void* data;
    u32 size;
    u32 numVertices;
    u32 bytesPerVertex;
    Primitive primitive;

    void* indices;
    u32 indicesSize;

    u64 attributes;
    u8 attributeCount;
    u16 attributeMask;
    u64 attributePermutations;
} VboData;

typedef struct {
    void* data;
    u32 width;
    u32 height;
    u32 size;
    PixelFormat format;
    u32 params;
} TextureData;

typedef struct {
    u16 rgbSources;
    u16 alphaSources;
    u16 rgbOperands;
    u16 alphaOperands;
    CombineFunc rgbCombine;
    CombineFunc alphaCombine;
    u32 constantColor;
} TexEnv;

static u32 bytesPerAttrFormat[] = {
    1, // ATTR_BYTE
    1, // ATTR_UNSIGNED_BYTE
    2, // ATTR_SHORT
    4, // ATTR_FLOAT
};

static u32 nibblesPerPixelFormat[] = {
    8, // RGBA8
    6, // RGB8
    4, // RGBA5551
    4, // RGB565
    4, // RGBA4
    4, // LA8
    4, // HILO8
    2, // L8
    2, // A8
    2, // LA4
    1, // L4
    1, // A4
    1, // ETC1
    2, // ETC1A4
};

static PixelFormat fbFormatToGPU[] = {
    PIXEL_RGBA8,    // GSP_RGBA8_OES
    PIXEL_RGB8,     // GSP_BGR8_OES
    PIXEL_RGB565,   // GSP_RGB565_OES
    PIXEL_RGBA5551, // GSP_RGB5_A1_OES
    PIXEL_RGBA4     // GSP_RGBA4_OES
};

extern Handle gspEvents[GSPEVENT_MAX];

u32 dirtyState;
u32 dirtyTexEnvs;
u32 dirtyTextures;

u32 clearColor;
u32 clearDepth;

Screen viewportScreen;
u32 viewportX;
u32 viewportY;
u32 viewportWidth;
u32 viewportHeight;

ScissorMode scissorMode;
u32 scissorX;
u32 scissorY;
u32 scissorWidth;
u32 scissorHeight;

float depthNear;
float depthFar;

CullMode cullMode;

bool stencilEnable;
TestFunc stencilFunc;
u8 stencilRef;
u8 stencilMask;
u8 stencilReplace;
StencilOp stencilFail;
StencilOp stencilZFail;
StencilOp stencilZPass;

u8 blendRed;
u8 blendGreen;
u8 blendBlue;
u8 blendAlpha;
BlendEquation blendColorEquation;
BlendEquation blendAlphaEquation;
BlendFactor blendColorSrc;
BlendFactor blendColorDst;
BlendFactor blendAlphaSrc;
BlendFactor blendAlphaDst;

bool alphaEnable;
TestFunc alphaFunc;
u8 alphaRef;

bool depthEnable;
TestFunc depthFunc;

u32 componentMask;

ShaderData* activeShader;

TexEnv texEnv[TEX_ENV_COUNT];

TextureData* activeTextures[TEX_UNIT_COUNT];
u32 enabledTextures;

bool allow3d;
ScreenSide screenSide;

u32* gpuCommandBuffer;
u32* gpuFrameBuffer;
u32* gpuDepthBuffer;

extern void gputInit();
extern void gputCleanup();

bool gpuInit() {
    if(!serviceRequire("gfx")) {
        return false;
    }

    dirtyState = 0xFFFFFFFF;
    dirtyTexEnvs = 0xFFFFFFFF;
    dirtyTextures = 0xFFFFFFFF;

    clearColor = 0;
    clearDepth = 0;

    viewportScreen = TOP_SCREEN;
    viewportX = 0;
    viewportY = 0;
    viewportWidth = TOP_WIDTH;
    viewportHeight = TOP_HEIGHT;

    scissorMode = SCISSOR_DISABLE;
    scissorX = 0;
    scissorY = 0;
    scissorWidth = TOP_WIDTH;
    scissorHeight = TOP_HEIGHT;

    depthNear = 0;
    depthFar = 1;

    cullMode = CULL_NONE;

    stencilEnable = false;
    stencilFunc = TEST_ALWAYS;
    stencilRef = 0;
    stencilMask = 0xFF;
    stencilReplace = 0;
    stencilFail = STENCIL_OP_KEEP;
    stencilZFail = STENCIL_OP_KEEP;
    stencilZPass = STENCIL_OP_KEEP;

    blendRed = 0;
    blendGreen = 0;
    blendBlue = 0;
    blendAlpha = 0;
    blendColorEquation = BLEND_ADD;
    blendAlphaEquation = BLEND_ADD;
    blendColorSrc = FACTOR_SRC_ALPHA;
    blendColorDst = FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAlphaSrc = FACTOR_SRC_ALPHA;
    blendAlphaDst = FACTOR_ONE_MINUS_SRC_ALPHA;

    alphaEnable = false;
    alphaFunc = TEST_ALWAYS;
    alphaRef = 0;

    depthEnable = false;
    depthFunc = TEST_GREATER;

    componentMask = GPU_WRITE_ALL;

    activeShader = NULL;

    texEnv[0].rgbSources = TEXENV_SOURCES(SOURCE_TEXTURE0, SOURCE_PRIMARY_COLOR, SOURCE_PRIMARY_COLOR);
    texEnv[0].alphaSources = TEXENV_SOURCES(SOURCE_TEXTURE0, SOURCE_PRIMARY_COLOR, SOURCE_PRIMARY_COLOR);
    texEnv[0].rgbOperands = TEXENV_OPERANDS(TEXENV_OP_RGB_SRC_COLOR, TEXENV_OP_RGB_SRC_COLOR, TEXENV_OP_RGB_SRC_COLOR);
    texEnv[0].alphaOperands = TEXENV_OPERANDS(TEXENV_OP_A_SRC_ALPHA, TEXENV_OP_A_SRC_ALPHA, TEXENV_OP_A_SRC_ALPHA);
    texEnv[0].rgbCombine = COMBINE_MODULATE;
    texEnv[0].alphaCombine = COMBINE_MODULATE;
    texEnv[0].constantColor = 0xFFFFFFFF;
    for(u8 env = 1; env < TEX_ENV_COUNT; env++) {
        texEnv[env].rgbSources = TEXENV_SOURCES(SOURCE_PREVIOUS, SOURCE_PRIMARY_COLOR, SOURCE_PRIMARY_COLOR);
        texEnv[env].alphaSources = TEXENV_SOURCES(SOURCE_PREVIOUS, SOURCE_PRIMARY_COLOR, SOURCE_PRIMARY_COLOR);
        texEnv[env].rgbOperands = TEXENV_OPERANDS(TEXENV_OP_RGB_SRC_COLOR, TEXENV_OP_RGB_SRC_COLOR, TEXENV_OP_RGB_SRC_COLOR);
        texEnv[env].alphaOperands = TEXENV_OPERANDS(TEXENV_OP_A_SRC_ALPHA, TEXENV_OP_A_SRC_ALPHA, TEXENV_OP_A_SRC_ALPHA);
        texEnv[env].rgbCombine = COMBINE_REPLACE;
        texEnv[env].alphaCombine = COMBINE_REPLACE;
        texEnv[env].constantColor = 0xFFFFFFFF;
    }

    for(u8 unit = 0; unit < TEX_UNIT_COUNT; unit++) {
        activeTextures[unit] = NULL;
    }

    enabledTextures = 0;

    allow3d = false;
    screenSide = LEFT_SCREEN;

    gpuCommandBuffer = (u32*) linearAlloc(GPU_COMMAND_BUFFER_SIZE * sizeof(u32));
    gpuFrameBuffer = (u32*) vramAlloc(TOP_WIDTH * TOP_HEIGHT * sizeof(u32));
    gpuDepthBuffer = (u32*) vramAlloc(TOP_WIDTH * TOP_HEIGHT * sizeof(u32));

    gfxSet3D(true);
    GPU_Init(NULL);
    GPU_Reset(NULL, gpuCommandBuffer, GPU_COMMAND_BUFFER_SIZE);

    gputInit();
    gpuClear();
    return true;
}

void gpuCleanup() {
    gputCleanup();

    if(gpuCommandBuffer != NULL) {
        linearFree(gpuCommandBuffer);
        gpuCommandBuffer = NULL;
    }

    if(gpuFrameBuffer != NULL) {
        vramFree(gpuFrameBuffer);
        gpuFrameBuffer = NULL;
    }

    if(gpuDepthBuffer != NULL) {
        vramFree(gpuDepthBuffer);
        gpuDepthBuffer = NULL;
    }
}

void* gpuAlloc(u32 size) {
    return linearAlloc(size);
}

void gpuFree(void* mem) {
    linearFree(mem);
}

void gpuUpdateState() {
    u32 dirtyUpdate = dirtyState;
    dirtyState = 0;

    if(dirtyUpdate & STATE_VIEWPORT) {
        GPU_SetViewport((u32*) osConvertVirtToPhys((u32) gpuDepthBuffer), (u32*) osConvertVirtToPhys((u32) gpuFrameBuffer), viewportX, viewportY, viewportHeight, viewportWidth);
    }

    if(dirtyUpdate & STATE_SCISSOR_TEST) {
        GPU_SetScissorTest((GPU_SCISSORMODE) scissorMode, scissorX, scissorY, scissorHeight, scissorWidth);
    }

    if(dirtyUpdate & STATE_DEPTH_MAP) {
        GPU_DepthMap(depthNear, depthFar);
    }

    if(dirtyUpdate & STATE_CULL) {
        GPU_SetFaceCulling((GPU_CULLMODE) cullMode);
    }

    if(dirtyUpdate & STATE_STENCIL_TEST) {
        GPU_SetStencilTest(stencilEnable, (GPU_TESTFUNC) stencilFunc, stencilRef, stencilMask, stencilReplace);
        GPU_SetStencilOp((GPU_STENCILOP) stencilFail, (GPU_STENCILOP) stencilZFail, (GPU_STENCILOP) stencilZPass);
    }

    if(dirtyUpdate & STATE_BLEND) {
        GPU_SetBlendingColor(blendRed, blendGreen, blendBlue, blendAlpha);
        GPU_SetAlphaBlending((GPU_BLENDEQUATION) blendColorEquation, (GPU_BLENDEQUATION) blendAlphaEquation, (GPU_BLENDFACTOR) blendColorSrc, (GPU_BLENDFACTOR) blendColorDst, (GPU_BLENDFACTOR) blendAlphaSrc, (GPU_BLENDFACTOR) blendAlphaDst);
    }

    if(dirtyUpdate & STATE_ALPHA_TEST) {
        GPU_SetAlphaTest(alphaEnable, (GPU_TESTFUNC) alphaFunc, alphaRef);
    }

    if(dirtyUpdate & STATE_DEPTH_TEST_AND_MASK) {
        GPU_SetDepthTestAndWriteMask(depthEnable, (GPU_TESTFUNC) depthFunc, (GPU_WRITEMASK) componentMask);
    }

    if((dirtyUpdate & STATE_ACTIVE_SHADER) && activeShader != NULL && activeShader->dvlb != NULL) {
        shaderProgramUse(&activeShader->program);
    }

    if((dirtyUpdate & STATE_TEX_ENV) && dirtyTexEnvs != 0) {
        for(u8 env = 0; env < TEX_ENV_COUNT; env++) {
            if(dirtyTexEnvs & (1 << env)) {
                GPU_SetTexEnv(env, texEnv[env].rgbSources, texEnv[env].alphaSources, texEnv[env].rgbOperands, texEnv[env].alphaOperands, (GPU_COMBINEFUNC) texEnv[env].rgbCombine, (GPU_COMBINEFUNC) texEnv[env].alphaCombine, texEnv[env].constantColor);
            }
        }

        dirtyTexEnvs = 0;
    }

    if((dirtyUpdate & STATE_TEXTURES) && dirtyTextures != 0) {
        for(u8 unit = 0; unit < TEX_UNIT_COUNT; unit++) {
            TexUnit texUnit = (TexUnit) (1 << unit);
            if(dirtyTextures & texUnit) {
                TextureData* textureData = activeTextures[unit];
                if(textureData != NULL && textureData->data != NULL) {
                    GPU_SetTexture((GPU_TEXUNIT) texUnit, (u32*) osConvertVirtToPhys((u32) textureData->data), (u16) textureData->width, (u16) textureData->height, textureData->params, (GPU_TEXCOLOR) textureData->format);
                    enabledTextures |= texUnit;
                } else {
                    enabledTextures &= ~texUnit;
                }
            }
        }

        GPU_SetTextureEnable((GPU_TEXUNIT) enabledTextures);
        dirtyTextures = 0;
    }
}

void gpuSafeWait(GSP_Event event) {
    Handle eventHandle = gspEvents[event];
    if(!svcWaitSynchronization(eventHandle, 40 * 1000 * 1000)) {
        svcClearEvent(eventHandle);
    }
}

void gpuFlush() {
    GPU_FinishDrawing();
    GPUCMD_Finalize();
    GPUCMD_FlushAndRun(NULL);
    gpuSafeWait(GSPEVENT_P3D);

    GPUCMD_SetBufferOffset(0);
}

void gpuFlushBuffer() {
    gfxScreen_t screen = viewportScreen == TOP_SCREEN ? GFX_TOP : GFX_BOTTOM;
    gfx3dSide_t side = allow3d && viewportScreen == TOP_SCREEN && screenSide == RIGHT_SCREEN ? GFX_RIGHT : GFX_LEFT;
    PixelFormat screenFormat = fbFormatToGPU[gfxGetScreenFormat(screen)];

    u16 fbWidth;
    u16 fbHeight;
    u32* fb = (u32*) gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

    GX_SetDisplayTransfer(NULL, gpuFrameBuffer, (viewportWidth << 16) | viewportHeight, fb, (fbHeight << 16) | fbWidth, GX_TRANSFER_OUT_FORMAT(screenFormat));
    gpuSafeWait(GSPEVENT_PPF);

    if(viewportScreen == TOP_SCREEN && !allow3d) {
        u16 fbWidthRight;
        u16 fbHeightRight;
        u32* fbRight = (u32*) gfxGetFramebuffer(screen, GFX_RIGHT, &fbWidthRight, &fbHeightRight);

        GX_SetDisplayTransfer(NULL, gpuFrameBuffer, (viewportWidth << 16) | viewportHeight, fbRight, (fbHeightRight << 16) | fbWidthRight, GX_TRANSFER_OUT_FORMAT(screenFormat));
        gpuSafeWait(GSPEVENT_PPF);
    }
}

void gpuSwapBuffers(bool vblank) {
    gfxSwapBuffersGpu();
    if(vblank) {
        gpuSafeWait(GSPEVENT_VBlank0);
    }
}

void gpuClearScreens() {
    u32 topSize = TOP_WIDTH * TOP_HEIGHT * nibblesPerPixelFormat[fbFormatToGPU[gfxGetScreenFormat(GFX_TOP)]] / 2;
    u32 bottomSize = BOTTOM_WIDTH * BOTTOM_HEIGHT * nibblesPerPixelFormat[fbFormatToGPU[gfxGetScreenFormat(GFX_BOTTOM)]] / 2;
    for(int buffer = 0; buffer < 2; buffer++) {
        u32* fbLeft = (u32*) gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
        memset(fbLeft, 0, topSize);

        u32* fbRight = (u32*) gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);
        memset(fbRight, 0, topSize);

        u32* fbBottom = (u32*) gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
        memset(fbBottom, 0, bottomSize);

        gpuSwapBuffers(false);
    }
}

void gpuClear() {
    GX_SetMemoryFill(NULL, gpuFrameBuffer, clearColor, &gpuFrameBuffer[viewportWidth * viewportHeight], 0x201, gpuDepthBuffer, clearDepth, &gpuDepthBuffer[viewportWidth * viewportHeight], 0x201);
    gpuSafeWait(GSPEVENT_PSC0);
}

void gpuClearColor(u8 red, u8 green, u8 blue, u8 alpha) {
    clearColor = (u32) (((red & 0xFF) << 24) | ((green & 0xFF) << 16) | ((blue & 0xFF) << 8) | (alpha & 0xFF));
}

void gpuClearDepth(u32 depth) {
    clearDepth = depth;
}

void gpuSet3d(bool enable3d) {
    allow3d = enable3d;
}

void gpuScreenSide(ScreenSide side) {
    screenSide = side;
}

int gpuGetViewportWidth() {
    return (int) viewportWidth;
}

int gpuGetViewportHeight() {
    return (int) viewportHeight;
}

void gpuViewport(Screen screen, u32 x, u32 y, u32 width, u32 height) {
    viewportScreen = screen;
    viewportX = x;
    viewportY = y;
    viewportWidth = width;
    viewportHeight = height;

    dirtyState |= STATE_VIEWPORT;
}

void gpuScissorTest(ScissorMode mode, u32 x, u32 y, u32 width, u32 height) {
    scissorMode = mode;
    scissorX = x;
    scissorY = y;
    scissorWidth = width;
    scissorHeight = height;

    dirtyState |= STATE_SCISSOR_TEST;
}

void gpuDepthMap(float near, float far) {
    depthNear = near;
    depthFar = far;

    dirtyState |= STATE_DEPTH_MAP;
}

void gpuCullMode(CullMode mode) {
    cullMode = mode;

    dirtyState |= STATE_CULL;
}

void gpuStencilTest(bool enable, TestFunc func, u8 ref, u8 mask, u8 replace) {
    stencilEnable = enable;
    stencilFunc = func;
    stencilRef = ref;
    stencilMask = mask;
    stencilReplace = replace;

    dirtyState |= STATE_STENCIL_TEST;
}

void gpuStencilOp(StencilOp fail, StencilOp zfail, StencilOp zpass) {
    stencilFail = fail;
    stencilZFail = zfail;
    stencilZPass = zpass;

    dirtyState |= STATE_STENCIL_TEST;
}

void gpuBlendColor(u8 red, u8 green, u8 blue, u8 alpha) {
    blendRed = red;
    blendGreen = green;
    blendBlue = blue;
    blendAlpha = alpha;

    dirtyState |= STATE_BLEND;
}

void gpuBlendFunc(BlendEquation colorEquation, BlendEquation alphaEquation, BlendFactor colorSrc, BlendFactor colorDst, BlendFactor alphaSrc, BlendFactor alphaDst) {
    blendColorEquation = colorEquation;
    blendAlphaEquation = alphaEquation;
    blendColorSrc = colorSrc;
    blendColorDst = colorDst;
    blendAlphaSrc = alphaSrc;
    blendAlphaDst = alphaDst;

    dirtyState |= STATE_BLEND;
}

void gpuAlphaTest(bool enable, TestFunc func, u8 ref) {
    alphaEnable = enable;
    alphaFunc = func;
    alphaRef = ref;

    dirtyState |= STATE_ALPHA_TEST;
}

void gpuDepthTest(bool enable, TestFunc func) {
    depthEnable = enable;
    depthFunc = func;

    dirtyState |= STATE_DEPTH_TEST_AND_MASK;
}

void gpuColorMask(bool red, bool green, bool blue, bool alpha) {
    componentMask = red ? componentMask | GPU_WRITE_RED : componentMask & ~GPU_WRITE_RED;
    componentMask = green ? componentMask | GPU_WRITE_GREEN : componentMask & ~GPU_WRITE_GREEN;
    componentMask = blue ? componentMask | GPU_WRITE_BLUE : componentMask & ~GPU_WRITE_BLUE;
    componentMask = alpha ? componentMask | GPU_WRITE_ALPHA : componentMask & ~GPU_WRITE_ALPHA;

    dirtyState |= STATE_DEPTH_TEST_AND_MASK;
}

void gpuDepthMask(bool depth) {
    componentMask = depth ? componentMask | GPU_WRITE_DEPTH : componentMask & ~GPU_WRITE_DEPTH;

    dirtyState |= STATE_DEPTH_TEST_AND_MASK;
}

void gpuCreateShader(u32* shader) {
    if(shader == NULL) {
        return;
    }

    *shader = (u32) malloc(sizeof(ShaderData));
    memset((void*) *shader, 0, sizeof(ShaderData));
}

void gpuFreeShader(u32 shader) {
    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL) {
        return;
    }

    if(shdr->dvlb != NULL) {
        shaderProgramFree(&shdr->program);
        DVLB_Free(shdr->dvlb);
    }

    free(shdr);
}

void gpuLoadShader(u32 shader, const void* data, u32 size, u8 geometryStride) {
    if(data == NULL) {
        return;
    }

    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL) {
        return;
    }

    if(shdr->dvlb != NULL) {
        shaderProgramFree(&shdr->program);
        DVLB_Free(shdr->dvlb);
    }

    shdr->dvlb = DVLB_ParseFile((u32*) data, size);
    shaderProgramInit(&shdr->program);
    if(shdr->dvlb->numDVLE > 0) {
        shaderProgramSetVsh(&shdr->program, &shdr->dvlb->DVLE[0]);
        if(shdr->dvlb->numDVLE > 1) {
            shaderProgramSetGsh(&shdr->program, &shdr->dvlb->DVLE[1], geometryStride);
        }
    }
}

void gpuUseShader(u32 shader) {
    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL || shdr->dvlb == NULL) {
        return;
    }

    activeShader = shdr;

    dirtyState |= STATE_ACTIVE_SHADER;
}

void gpuGetUniformBool(u32 shader, ShaderType type, int id, bool* value) {
    if(value == NULL) {
        return;
    }

    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL || shdr->dvlb == NULL) {
        return;
    }

    shaderInstance_s* instance = type == VERTEX_SHADER ? shdr->program.vertexShader : shdr->program.geometryShader;
    if(instance != NULL) {
        shaderInstanceGetBool(instance, id, value);
    }
}

void gpuSetUniformBool(u32 shader, ShaderType type, int id, bool value) {
    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL || shdr->dvlb == NULL) {
        return;
    }

    shaderInstance_s* instance = type == VERTEX_SHADER ? shdr->program.vertexShader : shdr->program.geometryShader;
    if(instance != NULL) {
        shaderInstanceSetBool(instance, id, value);
    }
}

void gpuSetUniform(u32 shader, ShaderType type, const char* name, const float* data, u32 elements) {
    if(name == NULL || data == NULL) {
        return;
    }

    ShaderData* shdr = (ShaderData*) shader;
    if(shdr == NULL || shdr->dvlb == NULL) {
        return;
    }

    float fixedData[elements * 4];
    for(u32 i = 0; i < elements; i++) {
        fixedData[i * 4 + 0] = data[i * 4 + 3];
        fixedData[i * 4 + 1] = data[i * 4 + 2];
        fixedData[i * 4 + 2] = data[i * 4 + 1];
        fixedData[i * 4 + 3] = data[i * 4 + 0];
    }

    shaderInstance_s* instance = type == VERTEX_SHADER ? shdr->program.vertexShader : shdr->program.geometryShader;
    if(instance != NULL) {
        Result res = shaderInstanceGetUniformLocation(instance, name);
        if(res >= 0) {
            GPU_SetFloatUniform((GPU_SHADER_TYPE) type, (u32) res, (u32*) fixedData, elements);
        }
    }
}

void gpuCreateVbo(u32* vbo) {
    if(vbo == NULL) {
        return;
    }

    *vbo = (u32) malloc(sizeof(VboData));
    memset((void*) *vbo, 0, sizeof(VboData));
}

void gpuFreeVbo(u32 vbo) {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return;
    }

    if(vboData->data != NULL) {
        linearFree(vboData->data);
    }

    if(vboData->indices != NULL) {
        linearFree(vboData->indices);
    }

    free(vboData);
}

void* gpuGetVboData(u32 vbo) {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return NULL;
    }

    return vboData->data;
}

void gpuVboDataInfo(u32 vbo, u32 numVertices, Primitive primitive) {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return;
    }

    u32 size = numVertices * vboData->bytesPerVertex;
    if(size != 0 && (vboData->data == NULL || vboData->size < size)) {
        if(vboData->data != NULL) {
            linearFree(vboData->data);
        }

        vboData->data = linearMemAlign(size, 0x80);
        if(vboData->data == NULL) {
            vboData->size = 0;
            return;
        }

        vboData->size = size;
    }

    vboData->numVertices = numVertices;
    vboData->primitive = primitive;
}

void gpuVboData(u32 vbo, const void* data, u32 numVertices, Primitive primitive) {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return;
    }

    gpuVboDataInfo(vbo, numVertices, primitive);
    if(data == NULL) {
        return;
    }

    u32 size = numVertices * vboData->bytesPerVertex;
    if(size > 0) {
        memcpy(vboData->data, data, size);
    }
}

void* gpuGetVboIndices(u32 vbo) {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return NULL;
    }

    return vboData->indices;
}

void gpuVboIndicesInfo(u32 vbo, u32 size) {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return;
    }

    if(size == 0) {
        if(vboData->indices != NULL) {
            linearFree(vboData->indices);
            vboData->indices = NULL;
            vboData->indicesSize = 0;
        }

        return;
    }

    if(vboData->indices == NULL || vboData->indicesSize < size) {
        if(vboData->indices != NULL) {
            linearFree(vboData->indices);
        }

        vboData->indices = linearMemAlign(size, 0x80);
        if(vboData->indices == NULL) {
            vboData->indicesSize = 0;
            return;
        }

        vboData->indicesSize = size;
    }
}

void gpuVboIndices(u32 vbo, const void* data, u32 size) {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return;
    }

    gpuVboIndicesInfo(vbo, data != NULL ? size : 0);
    if(data == NULL || size == 0) {
        return;
    }

    memcpy(vboData->indices, data, size);
}

void gpuVboAttributes(u32 vbo, u64 attributes, u8 attributeCount) {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL) {
        return;
    }

    vboData->attributes = attributes;
    vboData->attributeCount = attributeCount;
    vboData->attributeMask = 0xFFC;
    vboData->attributePermutations = 0;
    vboData->bytesPerVertex = 0;
    for(u32 i = 0; i < vboData->attributeCount; i++) {
        vboData->attributePermutations |= i << (i * 4);

        u8 data = (u8) ((attributes >> (i * 4)) & 0xF);
        u8 components = (u8) (((data >> 2) & 3) + 1);
        AttributeType type = (AttributeType) (data & 3);
        vboData->bytesPerVertex += components * bytesPerAttrFormat[type];
    }
}

void gpuDrawVbo(u32 vbo) {
    VboData* vboData = (VboData*) vbo;
    if(vboData == NULL || vboData->data == NULL) {
        return;
    }

    gpuUpdateState();

    static u32 attributeBufferOffset = 0;
    GPU_SetAttributeBuffers(vboData->attributeCount, (u32*) osConvertVirtToPhys((u32) vboData->data), vboData->attributes, vboData->attributeMask, vboData->attributePermutations, 1, &attributeBufferOffset, &vboData->attributePermutations, &vboData->attributeCount);
    if(vboData->indices != NULL) {
        GPU_DrawElements((GPU_Primitive_t) vboData->primitive, (u32*) vboData->indices, vboData->numVertices);
    } else {
        GPU_DrawArray((GPU_Primitive_t) vboData->primitive, vboData->numVertices);
    }
}

void gpuTexEnv(u32 env, u16 rgbSources, u16 alphaSources, u16 rgbOperands, u16 alphaOperands, CombineFunc rgbCombine, CombineFunc alphaCombine, u32 constantColor) {
    if(env >= TEX_ENV_COUNT) {
        return;
    }

    texEnv[env].rgbSources = rgbSources;
    texEnv[env].alphaSources = alphaSources;
    texEnv[env].rgbOperands = rgbOperands;
    texEnv[env].alphaOperands = alphaOperands;
    texEnv[env].rgbCombine = rgbCombine;
    texEnv[env].alphaCombine = alphaCombine;
    texEnv[env].constantColor = constantColor;

    dirtyState |= STATE_TEX_ENV;
    dirtyTexEnvs |= (1 << env);
}

void gpuCreateTexture(u32* texture) {
    if(texture == NULL) {
        return;
    }

    *texture = (u32) malloc(sizeof(TextureData));
    memset((void*) *texture, 0, sizeof(TextureData));
}

void gpuFreeTexture(u32 texture) {
    TextureData* textureData = (TextureData*) texture;
    if(textureData == NULL) {
        return;
    }

    if(textureData->data != NULL) {
        linearFree(textureData->data);
    }

    free(textureData);
}

void* gpuGetTextureData(u32 texture) {
    TextureData* textureData = (TextureData*) texture;
    if(textureData == NULL) {
        return NULL;
    }

    return textureData->data;
}

void gpuTextureInfo(u32 texture, u32 width, u32 height, PixelFormat format, u32 params) {
    TextureData* textureData = (TextureData*) texture;
    if(textureData == NULL || (textureData->data != NULL && width == textureData->width && height == textureData->height && format == textureData->format && params == textureData->params)) {
        return;
    }

    u32 size = (u32) (width * height * nibblesPerPixelFormat[format] / 2);
    if(textureData->data == NULL || textureData->size < size) {
        if(textureData->data != NULL) {
            linearFree(textureData->data);
        }

        textureData->data = linearMemAlign(size, 0x80);
        if(textureData->data == NULL) {
            textureData->size = 0;
            return;
        }

        textureData->size = size;
    }

    textureData->width = width;
    textureData->height = height;
    textureData->format = format;
    textureData->params = params;

    for(u8 unit = 0; unit < TEX_UNIT_COUNT; unit++) {
        if(activeTextures[unit] == textureData) {
            dirtyState |= STATE_TEXTURES;
            dirtyTextures |= (1 << unit);
        }
    }
}

void gpuTextureData(u32 texture, const void* data, u32 width, u32 height, PixelFormat format, u32 params) {
    if(data == NULL) {
        return;
    }

    TextureData* textureData = (TextureData*) texture;
    if(textureData == NULL) {
        return;
    }

    gpuTextureInfo(texture, width, height, format, params);

    GSPGPU_FlushDataCache(NULL, (u8*) data, (u32) (width * height * nibblesPerPixelFormat[format] / 2));
    GX_SetDisplayTransfer(NULL, (u32*) data, (height << 16) | width, (u32*) textureData->data, (height << 16) | width, (u32) (GX_TRANSFER_OUT_TILED(true) | GX_TRANSFER_IN_FORMAT(format) | GX_TRANSFER_OUT_FORMAT(format)));
    gpuSafeWait(GSPEVENT_PPF);
}

void gpuBindTexture(TexUnit unit, u32 texture) {
    u32 unitIndex = unit >> 1;
    if(activeTextures[unitIndex] != (TextureData*) texture) {
        activeTextures[unitIndex] = (TextureData*) texture;

        dirtyState |= STATE_TEXTURES;
        dirtyTextures |= (1 << unitIndex);
    }
}
