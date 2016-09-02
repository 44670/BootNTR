#ifndef __CTRCOMMON_GPU_HPP__
#define __CTRCOMMON_GPU_HPP__

#include "ctrcommon/types.hpp"

#include <string>

#define TOP_WIDTH 400
#define TOP_HEIGHT 240
#define BOTTOM_WIDTH 320
#define BOTTOM_HEIGHT 240

#define ATTRIBUTE(i, n, f) (((((n)-1)<<2)|((f)&3))<<((i)*4))

#define TEXENV_SOURCES(a,b,c) (((a))|((b)<<4)|((c)<<8))
#define TEXENV_OPERANDS(a,b,c) (((a))|((b)<<4)|((c)<<8))

#define TEXTURE_MAG_FILTER(v) (((v)&0x1)<<1)
#define TEXTURE_MIN_FILTER(v) (((v)&0x1)<<2)
#define TEXTURE_WRAP_S(v) (((v)&0x3)<<8)
#define TEXTURE_WRAP_T(v) (((v)&0x3)<<12)

typedef enum {
    TOP_SCREEN,
    BOTTOM_SCREEN
} Screen;

typedef enum {
    LEFT_SCREEN,
    RIGHT_SCREEN
} ScreenSide;

typedef enum {
    CULL_NONE = 0x0,
    CULL_FRONT_CCW = 0x1,
    CULL_BACK_CCW = 0x2
} CullMode;

typedef enum {
    TEST_NEVER = 0x0,
    TEST_ALWAYS = 0x1,
    TEST_EQUAL = 0x2,
    TEST_NOTEQUAL = 0x3,
    TEST_LESS = 0x4,
    TEST_LEQUAL = 0x5,
    TEST_GREATER = 0x6,
    TEST_GEQUAL = 0x7
} TestFunc;

typedef enum {
    STENCIL_OP_KEEP = 0x0,
    STENCIL_OP_AND_NOT = 0x1,
    STENCIL_OP_XOR = 0x5
} StencilOp;

typedef enum {
    BLEND_ADD = 0x0,
    BLEND_SUBTRACT = 0x1,
    BLEND_REVERSE_SUBTRACT = 0x2,
    BLEND_MIN = 0x3,
    BLEND_MAX = 0x4
} BlendEquation;

typedef enum {
    FACTOR_ZERO = 0x0,
    FACTOR_ONE = 0x1,
    FACTOR_SRC_COLOR = 0x2,
    FACTOR_ONE_MINUS_SRC_COLOR = 0x3,
    FACTOR_DST_COLOR = 0x4,
    FACTOR_ONE_MINUS_DST_COLOR = 0x5,
    FACTOR_SRC_ALPHA = 0x6,
    FACTOR_ONE_MINUS_SRC_ALPHA = 0x7,
    FACTOR_DST_ALPHA = 0x8,
    FACTOR_ONE_MINUS_DST_ALPHA = 0x9,
    FACTOR_CONSTANT_COLOR = 0xA,
    FACTOR_ONE_MINUS_CONSTANT_COLOR = 0xB,
    FACTOR_CONSTANT_ALPHA = 0xC,
    FACTOR_ONE_MINUS_CONSTANT_ALPHA = 0xD,
    FACTOR_SRC_ALPHA_SATURATE = 0xE
} BlendFactor;

typedef enum {
    VERTEX_SHADER = 0x0,
    GEOMETRY_SHADER = 0x1
} ShaderType;

typedef enum {
    PRIM_TRIANGLES = 0x0000,
    PRIM_TRIANGLE_STRIP = 0x0100,
    PRIM_TRIANGLE_FAN = 0x0200,
    PRIM_UNKPRIM = 0x0300
} Primitive;

typedef enum {
    ATTR_BYTE = 0x0,
    ATTR_UNSIGNED_BYTE = 0x1,
    ATTR_SHORT = 0x2,
    ATTR_FLOAT = 0x3
} AttributeType;

typedef enum{
    SOURCE_PRIMARY_COLOR = 0x0,
    SOURCE_TEXTURE0 = 0x3,
    SOURCE_TEXTURE1 = 0x4,
    SOURCE_TEXTURE2 = 0x5,
    SOURCE_TEXTURE3 = 0x6,
    SOURCE_CONSTANT = 0xE,
    SOURCE_PREVIOUS = 0xF
} TexEnvSource;

typedef enum {
    TEXENV_OP_RGB_SRC_COLOR = 0x00,
    TEXENV_OP_RGB_ONE_MINUS_SRC_COLOR = 0x01,
    TEXENV_OP_RGB_SRC_ALPHA = 0x02,
    TEXENV_OP_RGB_ONE_MINUS_SRC_ALPHA = 0x03,
    TEXENV_OP_RGB_SRC0_RGB = 0x04,
    TEXENV_OP_RGB_0x05 = 0x05,
    TEXENV_OP_RGB_0x06 = 0x06,
    TEXENV_OP_RGB_0x07 = 0x07,
    TEXENV_OP_RGB_SRC1_RGB = 0x08,
    TEXENV_OP_RGB_0x09 = 0x09,
    TEXENV_OP_RGB_0x0A = 0x0A,
    TEXENV_OP_RGB_0x0B = 0x0B,
    TEXENV_OP_RGB_SRC2_RGB = 0x0C,
    TEXENV_OP_RGB_0x0D = 0x0D,
    TEXENV_OP_RGB_0x0E = 0x0E,
    TEXENV_OP_RGB_0x0F = 0x0F,
} TexEnvOpRGB;

typedef enum {
    TEXENV_OP_A_SRC_ALPHA = 0x00,
    TEXENV_OP_A_ONE_MINUS_SRC_ALPHA = 0x01,
    TEXENV_OP_A_SRC0_RGB = 0x02,
    TEXENV_OP_A_SRC1_RGB = 0x04,
    TEXENV_OP_A_SRC2_RGB = 0x06,
} TexEnvOpA;

typedef enum {
    COMBINE_REPLACE = 0x0,
    COMBINE_MODULATE = 0x1,
    COMBINE_ADD = 0x2,
    COMBINE_ADD_SIGNED = 0x3,
    COMBINE_INTERPOLATE = 0x4,
    COMBINE_SUBTRACT = 0x5,
    COMBINE_DOT3_RGB = 0x6
} CombineFunc;

typedef enum {
    TEXUNIT0 = 0x1,
    TEXUNIT1 = 0x2,
    TEXUNIT2 = 0x4
} TexUnit;

typedef enum {
    FILTER_NEAREST = 0x0,
    FILTER_LINEAR = 0x1
} TextureFilter;

typedef enum {
    WRAP_CLAMP_TO_EDGE = 0x0,
    WRAP_REPEAT = 0x2
} TextureWrap;

typedef enum {
    PIXEL_RGBA8 = 0x0,
    PIXEL_RGB8 = 0x1,
    PIXEL_RGBA5551 = 0x2,
    PIXEL_RGB565 = 0x3,
    PIXEL_RGBA4 = 0x4,
    PIXEL_LA8 = 0x5,
    PIXEL_HILO8 = 0x6,
    PIXEL_L8 = 0x7,
    PIXEL_A8 = 0x8,
    PIXEL_LA4 = 0x9,
    PIXEL_L4 = 0xA,
    PIXEL_A4 = 0xB,
    PIXEL_ETC1 = 0xC,
    PIXEL_ETC1A4 = 0xD
} PixelFormat;

typedef enum {
    SCISSOR_DISABLE = 0x0,
    SCISSOR_INVERT = 0x1,
    SCISSOR_NORMAL = 0x3
} ScissorMode;

void* gpuAlloc(u32 size);
void gpuFree(void* mem);

void gpuFlush();
void gpuFlushBuffer();
void gpuSwapBuffers(bool vblank);

void gpuClearScreens();
void gpuClear();

void gpuClearColor(u8 red, u8 green, u8 blue, u8 alpha);
void gpuClearDepth(u32 depth);

void gpuSet3d(bool enable3d);
void gpuScreenSide(ScreenSide side);

int gpuGetViewportWidth();
int gpuGetViewportHeight();

void gpuViewport(Screen screen, u32 x, u32 y, u32 width, u32 height);
void gpuScissorTest(ScissorMode mode, u32 x, u32 y, u32 width, u32 height);
void gpuDepthMap(float near, float far);

void gpuCullMode(CullMode mode);

void gpuStencilTest(bool enable, TestFunc func, u8 ref, u8 mask, u8 replace);
void gpuStencilOp(StencilOp fail, StencilOp zfail, StencilOp zpass);

void gpuBlendColor(u8 red, u8 green, u8 blue, u8 alpha);
void gpuBlendFunc(BlendEquation colorEquation, BlendEquation alphaEquation, BlendFactor colorSrc, BlendFactor colorDst, BlendFactor alphaSrc, BlendFactor alphaDst);

void gpuAlphaTest(bool enable, TestFunc func, u8 ref);

void gpuDepthTest(bool enable, TestFunc func);

void gpuColorMask(bool red, bool green, bool blue, bool alpha);
void gpuDepthMask(bool depth);

void gpuCreateShader(u32* shader);
void gpuFreeShader(u32 shader);
void gpuLoadShader(u32 shader, const void* data, u32 size, u8 geometryStride = 0);
void gpuUseShader(u32 shader);
void gpuGetUniformBool(u32 shader, ShaderType type, int id, bool* value);
void gpuSetUniformBool(u32 shader, ShaderType type, int id, bool value);
void gpuSetUniform(u32 shader, ShaderType type, const char* name, const float* data, u32 elements);

void gpuCreateVbo(u32* vbo);
void gpuFreeVbo(u32 vbo);
void* gpuGetVboData(u32 vbo);
void gpuVboDataInfo(u32 vbo, u32 numVertices, Primitive primitive);
void gpuVboData(u32 vbo, const void* data, u32 numVertices, Primitive primitive);
void* gpuGetVboIndices(u32 vbo);
void gpuVboIndicesInfo(u32 vbo, u32 size);
void gpuVboIndices(u32 vbo, const void* data, u32 size);
void gpuVboAttributes(u32 vbo, u64 attributes, u8 attributeCount);
void gpuDrawVbo(u32 vbo);

inline u32 gpuTextureIndex(u32 x, u32 y, u32 w, u32 h) {
    return (((y >> 3) * (w >> 3) + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3));
}

void gpuTexEnv(u32 env, u16 rgbSources, u16 alphaSources, u16 rgbOperands, u16 alphaOperands, CombineFunc rgbCombine, CombineFunc alphaCombine, u32 constantColor);
void gpuCreateTexture(u32* texture);
void gpuFreeTexture(u32 texture);
void* gpuGetTextureData(u32 texture);
void gpuTextureInfo(u32 texture, u32 width, u32 height, PixelFormat format, u32 params);
void gpuTextureData(u32 texture, const void* data, u32 width, u32 height, PixelFormat format, u32 params);
void gpuBindTexture(TexUnit unit, u32 texture);

// GPUT - GPU Tools

void gputUseDefaultShader();

void gputMultMatrix4(float* out, const float* m1, const float* m2);
void gputIdentityMatrix(float *out);
void gputOrthoMatrix(float* out, float left, float right, float bottom, float top, float near, float far);
void gputPerspectiveMatrix(float* out, float fovy, float aspect, float near, float far);
void gputTranslationMatrix(float* out, float x, float y, float z);
void gputRotationMatrixX(float* out, float rotation);
void gputRotationMatrixY(float* out, float rotation);
void gputRotationMatrixZ(float* out, float rotation);
void gputScaleMatrix(float *out, float x, float y, float z);

void gputPushProjection();
void gputPopProjection();
float* gputGetProjection();
void gputProjection(float* matrix);
void gputOrtho(float left, float right, float bottom, float top, float near, float far);
void gputPerspective(float fovy, float aspect, float near, float far);

void gputPushModelView();
void gputPopModelView();
float* gputGetModelView();
void gputModelView(float* matrix);
void gputTranslate(float x, float y, float z);
void gputRotateX(float rotation);
void gputRotateY(float rotation);
void gputRotateZ(float rotation);
void gputRotate(float x, float y, float z);
void gputScale(float x, float y, float z);

float gputGetStringWidth(const std::string str, float charWidth);
float gputGetStringHeight(const std::string str, float charHeight);
void gputDrawString(const std::string str, float x, float y, float charWidth, float charHeight, u8 red = 0xFF, u8 green = 0xFF, u8 blue = 0xFF, u8 alpha = 0xFF);

void gputTakeScreenshot();

#endif