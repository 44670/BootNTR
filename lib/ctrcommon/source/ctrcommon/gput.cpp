#include "ctrcommon/gpu.hpp"
#include "ctrcommon/platform.hpp"

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <sstream>
#include <stack>

#include <3ds.h>
#include <math.h>

#include "ctrcommon_shader_vsh_shbin.h"
#include "ctrcommon_font_bin.h"

u32 defaultShader = 0;
u32 stringVbo = 0;
u32 dummyTexture = 0;
u32 fontTexture = 0;

float projection[16] = {0};
float modelview[16] = {0};

std::stack<float*> projectionStack;
std::stack<float*> modelviewStack;

void gputInit() {
    gpuCreateShader(&defaultShader);
    gpuLoadShader(defaultShader, ctrcommon_shader_vsh_shbin, ctrcommon_shader_vsh_shbin_size);
    gputUseDefaultShader();

    gpuCreateVbo(&stringVbo);
    gpuVboAttributes(stringVbo, ATTRIBUTE(0, 3, ATTR_FLOAT) | ATTRIBUTE(1, 2, ATTR_FLOAT) | ATTRIBUTE(2, 4, ATTR_FLOAT), 3);

    gpuCreateTexture(&dummyTexture);
    gpuTextureInfo(dummyTexture, 64, 64, PIXEL_RGBA8, TEXTURE_MIN_FILTER(FILTER_NEAREST) | TEXTURE_MAG_FILTER(FILTER_NEAREST));
    memset(gpuGetTextureData(dummyTexture), 0xFF, 64 * 64 * 4);

    void* gpuFont = gpuAlloc(ctrcommon_font_bin_size);
    memcpy(gpuFont, ctrcommon_font_bin, ctrcommon_font_bin_size);
    gpuCreateTexture(&fontTexture);
    gpuTextureData(fontTexture, gpuFont, 128, 128, PIXEL_RGBA8, TEXTURE_MIN_FILTER(FILTER_NEAREST) | TEXTURE_MAG_FILTER(FILTER_NEAREST));
    gpuFree(gpuFont);

    float identity[16];
    gputIdentityMatrix(identity);
    gputProjection(identity);
    gputModelView(identity);
}

void gputCleanup() {
    if(defaultShader != 0) {
        gpuFreeShader(defaultShader);
        defaultShader = 0;
    }

    if(stringVbo != 0) {
        gpuFreeVbo(stringVbo);
        stringVbo = 0;
    }

    if(fontTexture != 0) {
        gpuFreeTexture(fontTexture);
        fontTexture = 0;
    }
}

void gputUseDefaultShader() {
    gpuUseShader(defaultShader);
}

void gputMultMatrix4(float* out, const float* m1, const float* m2) {
    if(out == NULL || m1 == NULL || m2 == NULL) {
        return;
    }

    for(u32 x1 = 0; x1 < 4; x1++) {
        for(u32 y2 = 0; y2 < 4; y2++) {
            out[y2 * 4 + x1] = 0;
            for(u32 y1 = 0; y1 < 4; y1++) {
                out[y2 * 4 + x1] += m1[y1 * 4 + x1] * m2[y2 * 4 + y1];
            }
        }
    }
}

void gputIdentityMatrix(float *out) {
    if(out == NULL) {
        return;
    }

    memset(out, 0x00, 16 * sizeof(float));
    out[0] = 1.0f;
    out[5] = 1.0f;
    out[10] = 1.0f;
    out[15] = 1.0f;
}

void gputOrthoMatrix(float* out, float left, float right, float bottom, float top, float near, float far) {
    float orthoMatrix[16];

    orthoMatrix[0] = 2.0f / (right - left);
    orthoMatrix[1] = 0.0f;
    orthoMatrix[2] = 0.0f;
    orthoMatrix[3] = -((right + left) / (right - left));

    orthoMatrix[4] = 0.0f;
    orthoMatrix[5] = 2.0f / (top - bottom);
    orthoMatrix[6] = 0.0f;
    orthoMatrix[7] = -((top + bottom) / (top - bottom));

    orthoMatrix[8] = 0.0f;
    orthoMatrix[9] = 0.0f;
    orthoMatrix[10] = 2.0f / (far - near);
    orthoMatrix[11] = -((far + near) / (far - near));

    orthoMatrix[12] = 0.0f;
    orthoMatrix[13] = 0.0f;
    orthoMatrix[14] = 0.0f;
    orthoMatrix[15] = 1.0f;

    float correction[16];
    gputRotationMatrixZ(correction, (float) M_PI / 2.0f);

    gputMultMatrix4(out, orthoMatrix, correction);
}

void gputPerspectiveMatrix(float* out, float fovy, float aspect, float near, float far) {
    float top = near * (float) tan(fovy / 2);
    float right = top * aspect;

    float projectionMatrix[16];

    projectionMatrix[0] = near / right;
    projectionMatrix[1] = 0.0f;
    projectionMatrix[2] = 0.0f;
    projectionMatrix[3] = 0.0f;

    projectionMatrix[4] = 0.0f;
    projectionMatrix[5] = near / top;
    projectionMatrix[6] = 0.0f;
    projectionMatrix[7] = 0.0f;

    projectionMatrix[8] = 0.0f;
    projectionMatrix[9] = 0.0f;
    projectionMatrix[10] = -(far + near) / (far - near);
    projectionMatrix[11] = -2.0f * (far * near) / (far - near);

    projectionMatrix[12] = 0.0f;
    projectionMatrix[13] = 0.0f;
    projectionMatrix[14] = -1.0f;
    projectionMatrix[15] = 0.0f;

    float correction[16];
    gputIdentityMatrix(correction);
    correction[10] = 0.5f;
    correction[11] = -0.5f;

    gputMultMatrix4(out, correction, projectionMatrix);
}

void gputTranslationMatrix(float* out, float x, float y, float z) {
    if(out == NULL) {
        return;
    }

    gputIdentityMatrix(out);
    out[3] = x;
    out[7] = y;
    out[11] = z;
}

void gputRotationMatrixX(float* out, float rotation) {
    if(out == NULL) {
        return;
    }

    memset(out, 0x00, 16 * sizeof(float));

    out[0] = 1.0f;
    out[5] = (float) cos(rotation);
    out[6] = (float) sin(rotation);
    out[9] = (float) -sin(rotation);
    out[10] = (float) cos(rotation);
    out[15] = 1.0f;
}

void gputRotationMatrixY(float* out, float rotation) {
    if(out == NULL) {
        return;
    }

    memset(out, 0x00, 16 * sizeof(float));

    out[0] = (float) cos(rotation);
    out[2] = (float) sin(rotation);
    out[5] = 1.0f;
    out[8] = (float) -sin(rotation);
    out[10] = (float) cos(rotation);
    out[15] = 1.0f;
}

void gputRotationMatrixZ(float* out, float rotation) {
    if(out == NULL) {
        return;
    }

    memset(out, 0x00, 16 * sizeof(float));

    out[0] = (float) cos(rotation);
    out[1] = (float) sin(rotation);
    out[4] = (float) -sin(rotation);
    out[5] = (float) cos(rotation);
    out[10] = 1.0f;
    out[15] = 1.0f;
}

void gputScaleMatrix(float *matrix, float x, float y, float z) {
    matrix[0] *= x;
    matrix[4] *= x;
    matrix[8] *= x;
    matrix[12] *= x;

    matrix[1] *= y;
    matrix[5] *= y;
    matrix[9] *= y;
    matrix[13] *= y;

    matrix[2] *= z;
    matrix[6] *= z;
    matrix[10] *= z;
    matrix[14] *= z;
}

void gputPushProjection() {
    float* old = (float*) malloc(16 * sizeof(float));
    memcpy(old, projection, 16 * sizeof(float));
    projectionStack.push(old);
}

void gputPopProjection() {
    if(projectionStack.empty()) {
        return;
    }

    float* old = projectionStack.top();
    projectionStack.pop();
    gputProjection(old);
    free(old);
}

float* gputGetProjection() {
    return projection;
}

void gputProjection(float* matrix) {
    if(matrix == NULL) {
        return;
    }

    memcpy(projection, matrix, 16 * sizeof(float));
    gpuSetUniform(defaultShader, VERTEX_SHADER, "projection", projection, 4);
}

void gputOrtho(float left, float right, float bottom, float top, float near, float far) {
    float orthoMatrix[16];
    gputOrthoMatrix(orthoMatrix, left, right, bottom, top, near, far);
    gputProjection(orthoMatrix);
}

void gputPerspective(float fovy, float aspect, float near, float far) {
    float perspectiveMatrix[16];
    gputPerspectiveMatrix(perspectiveMatrix, fovy, aspect, near, far);
    gputProjection(perspectiveMatrix);
}

void gputPushModelView() {
    float* old = (float*) malloc(16 * sizeof(float));
    memcpy(old, modelview, 16 * sizeof(float));
    modelviewStack.push(old);
}

void gputPopModelView() {
    if(modelviewStack.empty()) {
        return;
    }

    float* old = modelviewStack.top();
    modelviewStack.pop();
    gputModelView(old);
    free(old);
}

float* gputGetModelView() {
    return modelview;
}

void gputModelView(float* matrix) {
    if(matrix == NULL) {
        return;
    }

    memcpy(modelview, matrix, 16 * sizeof(float));
    gpuSetUniform(defaultShader, VERTEX_SHADER, "modelview", modelview, 4);
}

void gputTranslate(float x, float y, float z) {
    float translationMatrix[16];
    gputTranslationMatrix(translationMatrix, x, y, z);

    float resultMatrix[16];
    gputMultMatrix4(resultMatrix, modelview, translationMatrix);
    gputModelView(resultMatrix);
}

void gputRotateX(float rotation) {
    float rotationMatrix[16];
    gputRotationMatrixX(rotationMatrix, rotation);

    float resultMatrix[16];
    gputMultMatrix4(resultMatrix, modelview, rotationMatrix);
    gputModelView(resultMatrix);
}

void gputRotateY(float rotation) {
    float rotationMatrix[16];
    gputRotationMatrixY(rotationMatrix, rotation);

    float resultMatrix[16];
    gputMultMatrix4(resultMatrix, modelview, rotationMatrix);
    gputModelView(resultMatrix);
}

void gputRotateZ(float rotation) {
    float rotationMatrix[16];
    gputRotationMatrixZ(rotationMatrix, rotation);

    float resultMatrix[16];
    gputMultMatrix4(resultMatrix, modelview, rotationMatrix);
    gputModelView(resultMatrix);
}

void gputRotate(float x, float y, float z) {
    float tempMatrix[16];
    float tempMatrix2[16];
    float tempMatrix3[16];

    gputRotationMatrixX(tempMatrix, x);
    gputRotationMatrixY(tempMatrix2, y);
    gputMultMatrix4(tempMatrix3, tempMatrix, tempMatrix2);

    gputRotationMatrixZ(tempMatrix2, z);
    gputMultMatrix4(tempMatrix, tempMatrix3, tempMatrix2);

    gputMultMatrix4(tempMatrix2, modelview, tempMatrix);
    gputModelView(tempMatrix2);
}

void gputScale(float x, float y, float z) {
    gputScaleMatrix(modelview, x, y, z);
    gputModelView(modelview);
}

float gputGetStringWidth(const std::string str, float charWidth) {
    u32 len = str.length();
    if(len == 0) {
        return 0;
    }

    u32 longestLine = 0;
    u32 currLength = 0;
    for(u32 i = 0; i < len; i++) {
        if(str[i] == '\n') {
            if(currLength > longestLine) {
                longestLine = currLength;
            }

            currLength = 0;
            continue;
        }

        currLength++;
    }

    if(currLength > longestLine) {
        longestLine = currLength;
    }

    return (int) (longestLine * charWidth);
}

float gputGetStringHeight(const std::string str, float charHeight) {
    u32 len = str.length();
    if(len == 0) {
        return 0;
    }

    u32 lines = 1;
    for(u32 i = 0; i < len; i++) {
        if(str[i] == '\n') {
            lines++;
        }
    }

    return (int) (lines * charHeight);
}

void gputDrawString(const std::string str, float x, float y, float charWidth, float charHeight, u8 red, u8 green, u8 blue, u8 alpha) {
    const u32 len = str.length();
    if(len == 0) {
        return;
    }

    static const float charSize = 8.0f / 128.0f;

    const float r = (float) red / 255.0f;
    const float g = (float) green / 255.0f;
    const float b = (float) blue / 255.0f;
    const float a = (float) alpha / 255.0f;

    gpuVboDataInfo(stringVbo, len * 6, PRIM_TRIANGLES);
    float* tempVboData = (float*) gpuGetVboData(stringVbo);

    float cx = x;
    float cy = y + gputGetStringHeight(str, charHeight) - charHeight;
    for(u32 i = 0; i < len; i++) {
        char c = str[i];
        if(c == '\n') {
            memset(tempVboData + (i * 6 * 9), 0, 6 * 9 * sizeof(float));

            cx = x;
            cy -= charHeight;
            continue;
        }

        const float texX1 = (c % 16) * charSize;
        const float texY1 = 1.0f - ((c / 16 + 1) * charSize);
        const float texX2 = texX1 + charSize;
        const float texY2 = texY1 + charSize;

        const float vboData[] = {
                cx, cy, -0.1f, texX1, texY1, r, g, b, a,
                cx + charWidth, cy, -0.1f, texX2, texY1, r, g, b, a,
                cx + charWidth, cy + charHeight, -0.1f, texX2, texY2, r, g, b, a,
                cx + charWidth, cy + charHeight, -0.1f, texX2, texY2, r, g, b, a,
                cx, cy + charHeight, -0.1f, texX1, texY2, r, g, b, a,
                cx, cy, -0.1f, texX1, texY1, r, g, b, a
        };

        memcpy(tempVboData + (i * 6 * 9), vboData, sizeof(vboData));
        cx += charWidth;
    }

    gpuBindTexture(TEXUNIT0, fontTexture);
    gpuDrawVbo(stringVbo);

    // Flush the GPU command buffer so we can safely reuse the VBO.
    gpuFlush();
}

void gputTakeScreenshot() {
    u32 headerSize = 0x36;
    u32 imageSize = 400 * 480 * 3;

    u8* header = (u8*) malloc(headerSize);
    memset(header, 0, headerSize);

    *(u16*) &header[0x0] = 0x4D42;
    *(u32*) &header[0x2] = headerSize + imageSize;
    *(u32*) &header[0xA] = headerSize;
    *(u32*) &header[0xE] = 0x28;
    *(u32*) &header[0x12] = 400;
    *(u32*) &header[0x16] = 480;
    *(u32*) &header[0x1A] = 0x00180001;
    *(u32*) &header[0x22] = imageSize;

    u8* image = (u8*) malloc(imageSize);
    memset(image, 0, imageSize);

    if(gfxGetScreenFormat(GFX_TOP) == GSP_BGR8_OES) {
        u8* top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
        for(u32 x = 0; x < 400; x++) {
            for(u32 y = 0; y < 240; y++) {
                u8* src = &top[((240 - y - 1) + x * 240) * 3];
                u8* dst = &image[((479 - y) * 400 + x) * 3];

                *(u16*) dst = *(u16*) src;
                dst[2] = src[2];
            }
        }
    }

    if(gfxGetScreenFormat(GFX_BOTTOM) == GSP_BGR8_OES) {
        u8* bottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
        for(u32 x = 0; x < 320; x++) {
            for(u32 y = 0; y < 240; y++) {
                u8* src = &bottom[((240 - y - 1) + x * 240) * 3];
                u8* dst = &image[((479 - (y + 240)) * 400 + (x + 40)) * 3];

                *(u16*) dst = *(u16*) src;
                dst[2] = src[2];
            }
        }
    }

    std::stringstream fileStream;
    fileStream << "/screenshot_" << platformGetTime() << ".bmp";
    std::string file = fileStream.str();

    FILE* fd = fopen(file.c_str(), "wb");
    if(fd) {
        fwrite(header, 1, headerSize, fd);
        fwrite(image, 1, imageSize, fd);
        fclose(fd);
    }

    free(header);
    free(image);
}