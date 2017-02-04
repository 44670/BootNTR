#include "draw.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <png.h>

#define PNG_SIGSIZE (8)

Result textureTile32(C3D_Tex *texture)
{
    u8      *tmp;
    int     i;
    int     height;
    int     width;
    u32     pixel;
    u32     size;
    
    height = (int)texture->height;
    width = (int)texture->width;
    tmp = linearAlloc(width * height * 4);
    if (!tmp) goto error;
    size = width * height * 4;
    for (i = 0; i < size; i += 4)
    {
        pixel = *(u32 *)(texture->data + i);
        *(u32 *)(tmp + i) = __builtin_bswap32(pixel);
    }    
    GSPGPU_FlushDataCache(tmp, width * height * 4);
    GSPGPU_FlushDataCache(texture->data, width * height * 4);
    C3D_SafeDisplayTransfer((u32 *)tmp, GX_BUFFER_DIM(width, height), \
        (u32*)texture->data, GX_BUFFER_DIM(width, height), TEXTURE_TRANSFER_FLAGS);
    gspWaitForPPF();
    linearFree(tmp);
    return (MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_COMMON, RD_SUCCESS));
error:
    return (MAKERESULT(RL_TEMPORARY, RS_OUTOFRESOURCE, RM_COMMON, RD_OUT_OF_MEMORY));
}

static void readPNGFile(png_structp pngPtr, png_bytep data, png_size_t length)
{
    FILE *file = (FILE *)png_get_io_ptr(pngPtr);
    fread(data, length, 1, file);
}

static Result loadPNGGeneric(sprite_t **out, const void *ioPtr)
{
    png_structp     pngPtr;
    png_infop       infoPtr;
    png_bytep       *rowPtrs;
    sprite_t        *sprite;
    Result          result;
    unsigned int    width;
    unsigned int    height;
    int             bitDepth;
    int             colorType;
    int             stride;
    int             i;

    rowPtrs = NULL;
    pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (pngPtr == NULL)
    {
        result = MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, RD_INVALID_RESULT_VALUE);
        goto errorCreateRead;
    }
    infoPtr = png_create_info_struct(pngPtr);
    if (infoPtr == NULL)
    {
        result = MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, RD_INVALID_RESULT_VALUE);
        goto errorCreateInfo;
    }
    if (setjmp(png_jmpbuf(pngPtr)))
    {
        png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)0);
        if (rowPtrs != NULL)
            free(rowPtrs);
        result = MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, RD_INVALID_RESULT_VALUE);
        return (result);
    }
    png_set_read_fn(pngPtr, (png_voidp)ioPtr, readPNGFile);
    png_set_sig_bytes(pngPtr, PNG_SIGSIZE);
    png_read_info(pngPtr, infoPtr);
    png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth, &colorType, NULL, NULL, NULL);
    if ((colorType == PNG_COLOR_TYPE_PALETTE && bitDepth <= 8)
        || (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
        || png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)
        || (bitDepth == 16))
    {
        png_set_expand(pngPtr);
    }

    if (bitDepth == 16) png_set_scale_16(pngPtr);
    if (bitDepth == 8 && colorType == PNG_COLOR_TYPE_RGB) png_set_filler(pngPtr, 0xFF, PNG_FILLER_AFTER);
    if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(pngPtr);
    if (colorType == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(pngPtr);
        png_set_filler(pngPtr, 0xFF, PNG_FILLER_AFTER);
    }
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) png_set_expand_gray_1_2_4_to_8(pngPtr);
    if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(pngPtr);
    if (bitDepth < 8) png_set_packing(pngPtr);
    png_read_update_info(pngPtr, infoPtr);
    rowPtrs = (png_bytep *)malloc(sizeof(png_bytep) * height);
    if (!rowPtrs)
    {
        result = MAKERESULT(RL_PERMANENT, RS_OUTOFRESOURCE, RM_APPLICATION, RD_OUT_OF_MEMORY);
        goto errorAllocRows;
    }
    sprite = newSprite(width, height);
    if (!sprite)
    {
        result = MAKERESULT(RL_PERMANENT, RS_OUTOFRESOURCE, RM_APPLICATION, RD_OUT_OF_MEMORY);
        goto errorCreateSprite;
    }
    else
        *out = sprite;
    stride = sprite->texture.width * 4;

    for (i = 0; i < height; i++)
    {
        rowPtrs[i] = (png_bytep)(sprite->texture.data + i * stride);
    }
    png_read_image(pngPtr, rowPtrs);
    textureTile32(&sprite->texture);
    result = MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS);
errorCreateSprite:
    free(rowPtrs);
errorAllocRows:
    png_destroy_info_struct(pngPtr, &infoPtr);
errorCreateInfo:
    png_destroy_read_struct(&pngPtr, (png_infopp)0, (png_infopp)0);
errorCreateRead:
    return (result);
}


Result  newSpriteFromPNG(sprite_t **out, const char *filename)
{
    FILE        *file;
    Result      result;
    png_byte    pngsig[PNG_SIGSIZE];

    if (!(file = fopen(filename, "rb")))
    {
        result = MAKERESULT(RL_PERMANENT, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
        goto exitError;
    }
    if (fread(pngsig, 1, PNG_SIGSIZE, file) != PNG_SIGSIZE)
    {
        result = MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_APPLICATION, RD_INVALID_SIZE);
        goto exitClose;
    }
    if (png_sig_cmp(pngsig, 0, PNG_SIGSIZE) != 0)
    {
        result = MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_APPLICATION, RD_INVALID_SELECTION);
        goto exitClose;
    }
    result = loadPNGGeneric(out, (void *)file);
exitClose:
    fclose(file);
exitError:
    return (result);
}
