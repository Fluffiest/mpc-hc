/*
 * Copyright (C) 2001-2003 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * Modified to support multi-thread related features
 * by Haruhiko Yamagata <h.yamagata@nifty.com> in 2006.
 */

#ifndef SWSCALE_SWSCALE_INTERNAL_H
#define SWSCALE_SWSCALE_INTERNAL_H

#include "../ffmpeg/mp_msg.h"

#ifdef __GNUC__
#define MSG_WARN(args...) mp_msg(MSGT_SWS,MSGL_WARN, ##args )
#define MSG_FATAL(args...) mp_msg(MSGT_SWS,MSGL_FATAL, ##args )
#define MSG_ERR(args...) mp_msg(MSGT_SWS,MSGL_ERR, ##args )
#define MSG_V(args...) mp_msg(MSGT_SWS,MSGL_V, ##args )
#define MSG_DBG2(args...) mp_msg(MSGT_SWS,MSGL_DBG2, ##args )
#define MSG_INFO(args...) mp_msg(MSGT_SWS,MSGL_INFO, ##args )
#define __align32(t,v) t v __attribute__ ((aligned (32)))
#else
#define MSG_WARN(args)
#define MSG_FATAL(args)
#define MSG_ERR(args)
#define MSG_V(args)
#define MSG_DBG2(args)
#define MSG_INFO(args)
#define __align32(t,v) __declspec(align(32)) t v
#endif

#define AV_TOSTRING(s) #s
#define STR(s)         AV_TOSTRING(s) //AV_STRINGIFY is too long

#define MAX_FILTER_SIZE 256

#if ARCH_X86_64
#   define APCK_PTR2 8
#   define APCK_COEF 16
#   define APCK_SIZE 24
#else
#   define APCK_PTR2 4
#   define APCK_COEF 8
#   define APCK_SIZE 16
#endif

typedef int (*SwsFunc)(struct SwsContext *context, uint8_t* src[], stride_t srcStride[], int srcSliceY,
                       int srcSliceH, uint8_t* dst[], stride_t dstStride[]);

struct SwsContext;

typedef struct SwsThreadParam
{
    struct SwsContext *c;
    uint8_t **src;
    stride_t* srcStride;
    int srcSliceY;
    int srcSliceH;
    uint8_t **dst;
    stride_t* dstStride;
    int dstYstart;
    int dstYend;
} SwsThreadParam;

/* This struct should be aligned on at least a 32-byte boundary. */
typedef struct SwsContext
{
    /**
     *
     * Note the src,dst,srcStride,dstStride will be copied, in the sws_scale() warper so they can freely be modified here
     */
    SwsFunc swScale;
    int srcW, srcH, dstH;
    int chrSrcW, chrSrcH, chrDstW, chrDstH;
    int lumXInc, chrXInc;
    int lumYInc, chrYInc;
    int dstFormat, srcFormat;               ///< format 4:2:0 type is allways YV12
    int origDstFormat, origSrcFormat;       ///< format
    int chrSrcHSubSample, chrSrcVSubSample;
    //int chrIntHSubSample, chrIntVSubSample;
    int chrDstHSubSample, chrDstVSubSample;
    int vChrDrop;
    int sliceDir;

    void *thread_opaque;
    int thread_count;
    int (*execute)(struct SwsContext *c, int (*func)(struct SwsContext *c), int *ret, int count);
    int *ret;
    SwsThreadParam stp;
    int16_t **lumPixBuf;
    int16_t **chrPixBuf;
    int16_t *hLumFilter;
    int16_t *hLumFilterPos;
    int16_t *hChrFilter;
    int16_t *hChrFilterPos;
    int16_t *vLumFilter;
    int16_t *vLumFilterPos;
    int16_t *vChrFilter;
    int16_t *vChrFilterPos;

    __align32(uint8_t, formatConvBuffer[8000]); //FIXME dynamic alloc, but we have to change alot of code for this to be usefull

    int hLumFilterSize;
    int hChrFilterSize;
    int vLumFilterSize;
    int vChrFilterSize;
    int vLumBufSize;
    int vChrBufSize;

    uint8_t *funnyYCode;
    uint8_t *funnyUVCode;
    int32_t *lumMmx2FilterPos;
    int32_t *chrMmx2FilterPos;
    int16_t *lumMmx2Filter;
    int16_t *chrMmx2Filter;

    int canMMX2BeUsed;

    int lastInLumBuf;
    int lastInChrBuf;
    int lumBufIndex;
    int chrBufIndex;
    int dstY;
    SwsParams params;
    void * yuvTable;			// pointer to the yuv->rgb table start so it can be freed()
    unsigned char * table_rV[256];
    unsigned char * table_gU[256];
    int    table_gV[256];
    unsigned char * table_bU[256];

    //Colorspace stuff
    int contrast, brightness, saturation;	// for sws_getColorspaceDetails
    int srcColorspaceTable[7];
    int dstColorspaceTable[7];
    int srcRange, dstRange;

#define RED_DITHER            "0*8"
#define GREEN_DITHER          "1*8"
#define BLUE_DITHER           "2*8"
#define Y_COEFF               "3*8"
#define VR_COEFF              "4*8"
#define UB_COEFF              "5*8"
#define VG_COEFF              "6*8"
#define UG_COEFF              "7*8"
#define Y_OFFSET              "8*8"
#define U_OFFSET              "9*8"
#define V_OFFSET              "10*8"
#define LUM_MMX_FILTER_OFFSET "11*8"
#define CHR_MMX_FILTER_OFFSET "11*8+4*4*256"
#define DSTW_OFFSET           "11*8+4*4*256*2" //do not change, it is hardcoded in the ASM
#define ESP_OFFSET            "11*8+4*4*256*2+8"
#define VROUNDER_OFFSET       "11*8+4*4*256*2+16"
#define U_TEMP                "11*8+4*4*256*2+24"
#define V_TEMP                "11*8+4*4*256*2+32"

    uint64_t redDither   __attribute__((aligned(8)));
    uint64_t greenDither __attribute__((aligned(8)));
    uint64_t blueDither  __attribute__((aligned(8)));

    uint64_t yCoeff      __attribute__((aligned(8)));
    uint64_t vrCoeff     __attribute__((aligned(8)));
    uint64_t ubCoeff     __attribute__((aligned(8)));
    uint64_t vgCoeff     __attribute__((aligned(8)));
    uint64_t ugCoeff     __attribute__((aligned(8)));
    uint64_t yOffset     __attribute__((aligned(8)));
    uint64_t uOffset     __attribute__((aligned(8)));
    uint64_t vOffset     __attribute__((aligned(8)));
    int32_t  lumMmxFilter[4*MAX_FILTER_SIZE];
    int32_t  chrMmxFilter[4*MAX_FILTER_SIZE];
    int dstW;
    uint64_t esp __attribute__((aligned(8)));
    uint64_t vRounder     __attribute__((aligned(8)));
    uint64_t u_temp       __attribute__((aligned(8)));
    uint64_t v_temp       __attribute__((aligned(8)));

} SwsContext;

//FIXME check init (where 0)

SwsFunc yuv2rgb_get_func_ptr(SwsContext *c);
int yuv2rgb_c_init_tables(SwsContext *c, const int inv_table[7], int fullRange, int brightness, int contrast, int saturation);

char *sws_format_name(int format);

#endif /* SWSCALE_SWSCALE_INTERNAL_H */
