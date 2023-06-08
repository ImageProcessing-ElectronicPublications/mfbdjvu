/*
https://github.com/plzombie/depress/issues/2
*/

#ifndef DJVUL_H_
#define DJVUL_H_
#define DJVUL_VERSION "3.1"

#include <stdbool.h>
#include <math.h>

#ifdef DJVUL_STATIC
#define DJVULAPI static
#else
#define DJVULAPI extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

DJVULAPI int ImageDjvulThreshold(unsigned char* buf, bool* bufmask, unsigned char* bufbg, unsigned char* buffg, unsigned int width, unsigned int height, unsigned int channels, unsigned int bgs, unsigned int level, int wbmode, float doverlay, float anisotropic, float contrast, float fbscale, float delta);
DJVULAPI int ImageDjvulGround(unsigned char* buf, bool* bufmask, unsigned char* bufbg, unsigned char* buffg, unsigned int width, unsigned int height, unsigned int channels, unsigned int bgs, unsigned int level, float doverlay);
DJVULAPI int ImageFGdownsample(unsigned char* buffg, unsigned int width, unsigned int height, unsigned int channels, unsigned int fgs);
DJVULAPI int ImageDjvuReconstruct(unsigned char* buf, bool* bufmask, unsigned char* bufbg, unsigned char* buffg, unsigned int width, unsigned int height, unsigned int channels, unsigned int widthbg, unsigned int heightbg, unsigned int widthfg, unsigned int heightfg);
DJVULAPI int ImageDjvulSelect(unsigned char* buf, bool* bufmask, unsigned char* bufbg, unsigned char* buffg, unsigned int width, unsigned int height, unsigned int channels, unsigned int bgs, unsigned int level, int wbmode, float doverlay, float anisotropic, float contrast, float fbscale, float delta, float radius, float sensitivity, int method);

#ifdef __cplusplus
}
#endif

#define DJVUL_IMAGE_CHANNELS 3
#define TDJVUL 0
#define TBIMOD 1
#define TSAUVOLA 2
#define TBLUR 3
#define TEDGEPLUS 4

#ifdef DJVUL_IMPLEMENTATION

#include "threshold.h"

/*
ImageDjvulThreshold()

input:
buf - unsigned char* image (height * width * channels)
bgs = 3 // downscale BG and FG
level = 0 [auto]
wbmode = 1 [white]
doverlay = 0.5f [half]
anisotropic = 0.0f [off, regulator]
contrast = 0.0f [off, regulator]
fbscale = 1.0f [off, regulator]
delta = 0.0f [off, regulator]

output:
bufmask - bool* image mask (height * width)
bufbg, buffg - unsigned char* BG, FG (heightbg * widthbg * channels, heightbg = (height + bgs - 1) / bgs, widthbg = (width + bgs - 1) / bgs)
level - use level

Use:
int level = ImageDjvulThreshold(buf, bufmask, bufbg, buffg, width, height, channels, bgs, level, wbmode, doverlay, anisotropic, contrast, fbscale, delta);
*/

DJVULAPI int ImageDjvulThreshold(unsigned char* buf, bool* bufmask, unsigned char* bufbg, unsigned char* buffg, unsigned int width, unsigned int height, unsigned int channels, unsigned int bgs, unsigned int level, int wbmode, float doverlay, float anisotropic, float contrast, float fbscale, float delta)
{
    unsigned int y, x, d, i, j, mchannels;
    unsigned int y0, x0, y1, x1, y0b, x0b, y1b, x1b, yb, xb;
    unsigned int widthbg, heightbg, whcp, blsz;
    unsigned long k, l, lm, n;
    unsigned char fgbase, bgbase;
    unsigned int cnth, cntw;
    int pim[DJVUL_IMAGE_CHANNELS], gim[DJVUL_IMAGE_CHANNELS], tim[DJVUL_IMAGE_CHANNELS];
    int fgim[DJVUL_IMAGE_CHANNELS], bgim[DJVUL_IMAGE_CHANNELS];
    int imd;
    float fgk, imx, partl, parts, ims[DJVUL_IMAGE_CHANNELS];
    float fgdist, bgdist, fgdistf, bgdistf, kover, fgpart, bgpart;
    unsigned int maskbl, maskover, bgsover;
    unsigned long int fgnum, bgnum;
    unsigned long int fgsum[DJVUL_IMAGE_CHANNELS], bgsum[DJVUL_IMAGE_CHANNELS];

    mchannels = (channels < DJVUL_IMAGE_CHANNELS) ? channels : DJVUL_IMAGE_CHANNELS;
    if (bgs > 0)
    {
        widthbg = (width + bgs - 1) / bgs;
        heightbg = (height + bgs - 1) / bgs;
    }
    else
    {
        return 0;
    }

    // level calculation
    whcp = height;
    whcp += width;
    whcp /= 2;
    blsz = 1;
    if (level == 0)
    {
        while (bgs * blsz < whcp)
        {
            level++;
            blsz <<= 1;
        }
    }
    else
    {
        for (l = 0; l < level; l++)
        {
            blsz <<= 1;
        }
    }
    blsz >>= 1;
    if (doverlay < 0.0f)
    {
        doverlay = 0.0f;
    }
    kover = doverlay + 1.0f;

    // w/b mode {1/-1}
    if (wbmode < 0)
    {
        fgbase = 255;
        bgbase = 0;
    }
    else
    {
        fgbase = 0;
        bgbase = 255;
    }
    k = 0;
    for (y = 0; y < heightbg; y++)
    {
        for (x = 0; x < widthbg; x++)
        {
            for (d = 0; d < mchannels; d++)
            {
                buffg[k] = fgbase;
                bufbg[k] = bgbase;
                k++;
            }
        }
    }

    // level blocks
    for (l = 0; l < level; l++)
    {
        cnth = (heightbg + blsz - 1) / blsz;
        cntw = (widthbg + blsz - 1) / blsz;
        maskbl = bgs * blsz;
        maskover = (unsigned int)(kover * maskbl);
        bgsover = (unsigned int)(kover * blsz);
        partl = (float)(level - l) / (float)level;
        for (i = 0; i < cnth; i++)
        {
            y0 = i * maskbl;
            y1 = (((y0 + maskover) < height) ? (y0 + maskover) : height);
            y0b = i * blsz;
            y1b = (((y0b + bgsover) < heightbg) ? (y0b + bgsover) : heightbg);
            for (j = 0; j < cntw; j++)
            {
                x0 = j * maskbl;
                x1 = (((x0 + maskover) < width) ? (x0 + maskover) : width);
                x0b = j * blsz;
                x1b = (((x0b + bgsover) < widthbg) ? (x0b + bgsover) : widthbg);

                // mean region buf
                for (d = 0; d < mchannels; d++)
                {
                    ims[d] = 0.0f;
                }
                n = 0;
                k = 0;
                for (y = y0; y < y1; y++)
                {
                    for (x = x0; x < x1; x++)
                    {
                        k = (width * y + x) * channels;
                        for (d = 0; d < mchannels; d++)
                        {
                            ims[d] += (float)buf[k + d];
                        }
                        n++;
                    }
                }
                for (d = 0; d < mchannels; d++)
                {
                    if (n > 0)
                    {
                        ims[d] /= (float)n;
                    }
                    ims[d] += 0.5f;
                    ims[d] = (ims[d] < 0.0f) ? 0.0f : (ims[d] < 255.0f) ? ims[d] : 255.0f;
                    gim[d] = (int)ims[d];
                }

                // mean region buffg
                for (d = 0; d < mchannels; d++)
                {
                    ims[d] = 0.0f;
                }
                n = 0;
                k = 0;
                for (y = y0b; y < y1b; y++)
                {
                    for (x = x0b; x < x1b; x++)
                    {
                        k = (widthbg * y + x) * channels;
                        for (d = 0; d < mchannels; d++)
                        {
                            ims[d] += (float)buffg[k + d];
                        }
                        n++;
                    }
                }
                for (d = 0; d < mchannels; d++)
                {
                    if (n > 0)
                    {
                        ims[d] /= (float)n;
                    }
                    ims[d] += 0.5f;
                    ims[d] = (ims[d] < 0.0f) ? 0.0f : (ims[d] < 255.0f) ? ims[d] : 255.0f;
                    fgim[d] = (int)ims[d];
                }

                // mean region bufbg
                for (d = 0; d < mchannels; d++)
                {
                    ims[d] = 0.0f;
                }
                n = 0;
                k = 0;
                for (y = y0b; y < y1b; y++)
                {
                    for (x = x0b; x < x1b; x++)
                    {
                        k = (widthbg * y + x) * channels;
                        for (d = 0; d < mchannels; d++)
                        {
                            ims[d] += (float)bufbg[k + d];
                        }
                        n++;
                    }
                }
                for (d = 0; d < mchannels; d++)
                {
                    if (n > 0)
                    {
                        ims[d] /= (float)n;
                    }
                    ims[d] += 0.5f;
                    ims[d] = (ims[d] < 0.0f) ? 0.0f : (ims[d] < 255.0f) ? ims[d] : 255.0f;
                    bgim[d] = (int)ims[d];
                }

                // distance buffg -> buf, bufbg -> buf
                fgdist = 0.0f;
                for (d = 0; d < mchannels; d++)
                {
                    imd = gim[d];
                    imd -= fgim[d];
                    if (imd < 0) imd = -imd;
                    fgdist += imd;
                }
                bgdist = 0.0f;
                for (d = 0; d < mchannels; d++)
                {
                    imd = gim[d];
                    imd -= bgim[d];
                    if (imd < 0) imd = -imd;
                    bgdist += imd;
                }

                // anisotropic regulator
                fgk = (fgdist + bgdist);
                if (fgk > 0.0f)
                {
                    fgk = (bgdist - fgdist) / fgk;
                    fgk *= anisotropic;
                    fgk = (float)(exp(fgk));
                }
                else
                {
                    fgk = 1.0f;
                }
                fgk *= fbscale;

                // separate FG and BG
                for (d = 0; d < mchannels; d++)
                {
                    fgsum[d] = 0;
                    bgsum[d] = 0;
                }
                fgnum = 0;
                bgnum = 0;
                for (y = y0; y < y1; y++)
                {
                    for (x = x0; x < x1; x++)
                    {
                        k = (width * y + x) * channels;
                        for (d = 0; d < mchannels; d++)
                        {
                            pim[d] = (int)buf[k + d];
                            tim[d] = pim[d] +  (int)(contrast * (pim[d] - gim[d]));
                        }

                        fgdistf = 0.0f;
                        for (d = 0; d < mchannels; d++)
                        {
                            imd = tim[d];
                            imd -= fgim[d];
                            if (imd < 0) imd = -imd;
                            fgdistf += imd;
                        }
                        bgdistf = 0.0f;
                        for (d = 0; d < mchannels; d++)
                        {
                            imd = tim[d];
                            imd -= bgim[d];
                            if (imd < 0) imd = -imd;
                            bgdistf += imd;
                        }

                        if ((fgdistf * fgk + delta) < bgdistf)
                        {
                            for (d = 0; d < mchannels; d++)
                            {
                                fgsum[d] += (unsigned long int)pim[d];
                            }
                            fgnum++;
                        }
                        else
                        {
                            for (d = 0; d < mchannels; d++)
                            {
                                bgsum[d] += (unsigned long int)pim[d];
                            }
                            bgnum++;
                        }
                    }
                }
                if (fgnum > 0)
                {
                    for (d = 0; d < mchannels; d++)
                    {
                        fgsum[d] += (fgnum >> 1);
                        fgsum[d] /= fgnum;
                        fgim[d] = (int)fgsum[d];
                    }
                }
                if (bgnum > 0)
                {
                    for (d = 0; d < mchannels; d++)
                    {
                        bgsum[d] += (bgnum >> 1);
                        bgsum[d] /= bgnum;
                        bgim[d] = (int)bgsum[d];
                    }
                }

                fgpart = 1.0f;
                bgpart = 1.0f;
                if ((fgdist + bgdist) > 0.0f)
                {
                    fgpart += (fgdist + fgdist) / (fgdist + bgdist);
                    bgpart += (bgdist + bgdist) / (fgdist + bgdist);
                }
                fgpart *= partl;
                bgpart *= partl;

                // average old and new FG
                parts = 1.0f /(fgpart + 1.0f);
                for (y = y0b; y < y1b; y++)
                {
                    for (x = x0b; x < x1b; x++)
                    {
                        k = (widthbg * y + x) * channels;
                        for (d = 0; d < mchannels; d++)
                        {
                            imx = (float)buffg[k + d];
                            imx *= fgpart;
                            imx += (float)fgim[d];
                            imx *= parts;
                            imx += 0.5f;
                            imx = (imx < 0.0f) ? 0.0f : (imx < 255.0f) ? imx : 255.0f;
                            buffg[k + d] = (unsigned char)imx;
                        }
                    }
                }

                // average old and new BG
                parts = 1.0f /(bgpart + 1.0f);
                for (y = y0b; y < y1b; y++)
                {
                    for (x = x0b; x < x1b; x++)
                    {
                        k = (widthbg * y + x) * channels;
                        for (d = 0; d < mchannels; d++)
                        {
                            imx = (float)bufbg[k + d];
                            imx *= bgpart;
                            imx += (float)bgim[d];
                            imx *= parts;
                            imx += 0.5f;
                            imx = (imx < 0.0f) ? 0.0f : (imx < 255.0f) ? imx : 255.0f;
                            bufbg[k + d] = (unsigned char)imx;
                        }
                    }
                }
            }
        }
        blsz >>= 1;
    }

    // threshold mask
    l = 0;
    lm = 0;
    for (y = 0; y < height; y++)
    {
        yb = y / bgs;
        for (x = 0; x < width; x++)
        {
            xb = x / bgs;
            k = (widthbg * yb + xb) * channels;
            for (d = 0; d < mchannels; d++)
            {
                pim[d] = (int)buf[l + d];
                fgim[d] = (int)buffg[k + d];
                bgim[d] = (int)bufbg[k + d];
            }
            l += channels;

            // distance buffg -> buf, bufbg -> buf
            fgdist = 0.0f;
            for (d = 0; d < mchannels; d++)
            {
                imd = pim[d];
                imd -= fgim[d];
                if (imd < 0) imd = -imd;
                fgdist += (float)imd;
            }
            bgdist = 0.0f;
            for (d = 0; d < mchannels; d++)
            {
                imd = pim[d];
                imd -= bgim[d];
                if (imd < 0) imd = -imd;
                bgdist += (float)imd;
            }
            bufmask[lm] = (fgdist < bgdist);
            lm++;
        }
    }

    return level;
}

/*
ImageDjvulGround()

input:
buf - unsigned char* image (height * width * channels)
bufmask - bool* image mask (height * width)
bgs = 3 // downscale BG and FG
level = 0 [auto]
doverlay = 0.5f [half]

output:
bufbg, buffg - unsigned char* BG, FG (heightbg * widthbg * channels, heightbg = (height + bgs - 1) / bgs, widthbg = (width + bgs - 1) / bgs)
level - use level

Use:
int level = ImageDjvulThreshold(buf, bufmask, bufbg, buffg, width, height, bgs, level, doverlay);
*/

DJVULAPI int ImageDjvulGround(unsigned char* buf, bool* bufmask, unsigned char* bufbg, unsigned char* buffg, unsigned int width, unsigned int height, unsigned int channels, unsigned int bgs, unsigned int level, float doverlay)
{
    unsigned int y, x, d, i, j, mchannels;
    unsigned int y0, x0, y1, x1, y0b, x0b, y1b, x1b, yb, xb;
    unsigned int widthbg, heightbg, whcp, blsz;
    unsigned long k, km, l, lm, n;
    unsigned char fgbase, bgbase;
    unsigned int cnth, cntw;
    int pim[DJVUL_IMAGE_CHANNELS], fgim[DJVUL_IMAGE_CHANNELS], bgim[DJVUL_IMAGE_CHANNELS];
    int imd;
    bool mim;
    float imx, partl, parts, ims[DJVUL_IMAGE_CHANNELS];
    float fgdist, bgdist, kover, fgpart, bgpart;
    unsigned int maskbl, maskover, bgsover;
    unsigned long int fgnum, bgnum;
    unsigned long int fgsum[DJVUL_IMAGE_CHANNELS], bgsum[DJVUL_IMAGE_CHANNELS];

    mchannels = (channels < DJVUL_IMAGE_CHANNELS) ? channels : DJVUL_IMAGE_CHANNELS;
    if (bgs > 0)
    {
        widthbg = (width + bgs - 1) / bgs;
        heightbg = (height + bgs - 1) / bgs;
    }
    else
    {
        return 0;
    }

    // level calculation
    whcp = height;
    whcp += width;
    whcp /= 2;
    blsz = 1;
    if (level == 0)
    {
        while (bgs * blsz < whcp)
        {
            level++;
            blsz <<= 1;
        }
    }
    else
    {
        for (l = 0; l < level; l++)
        {
            blsz <<= 1;
        }
    }
    blsz >>= 1;
    if (doverlay < 0.0f)
    {
        doverlay = 0.0f;
    }
    kover = doverlay + 1.0f;

    fgbase = 127;
    bgbase = 127;
    k = 0;
    for (y = 0; y < heightbg; y++)
    {
        for (x = 0; x < widthbg; x++)
        {
            for (d = 0; d < mchannels; d++)
            {
                buffg[k] = fgbase;
                bufbg[k] = bgbase;
                k++;
            }
        }
    }

    // level blocks
    for (l = 0; l < level; l++)
    {
        cnth = (heightbg + blsz - 1) / blsz;
        cntw = (widthbg + blsz - 1) / blsz;
        maskbl = bgs * blsz;
        maskover = (unsigned int)(kover * maskbl);
        bgsover = (unsigned int)(kover * blsz);
        partl = (float)(level - l) / (float)level;
        for (i = 0; i < cnth; i++)
        {
            y0 = i * maskbl;
            y1 = (((y0 + maskover) < height) ? (y0 + maskover) : height);
            y0b = i * blsz;
            y1b = (((y0b + bgsover) < heightbg) ? (y0b + bgsover) : heightbg);
            for (j = 0; j < cntw; j++)
            {
                x0 = j * maskbl;
                x1 = (((x0 + maskover) < width) ? (x0 + maskover) : width);
                x0b = j * blsz;
                x1b = (((x0b + bgsover) < widthbg) ? (x0b + bgsover) : widthbg);

                // separate FG and BG
                for (d = 0; d < mchannels; d++)
                {
                    fgsum[d] = 0;
                    bgsum[d] = 0;
                }
                fgnum = 0;
                bgnum = 0;
                for (y = y0; y < y1; y++)
                {
                    for (x = x0; x < x1; x++)
                    {
                        km = width * y + x;
                        k = km * channels;
                        for (d = 0; d < mchannels; d++)
                        {
                            pim[d] = (int)buf[k + d];
                        }
                        mim = bufmask[km];

                        if (mim)
                        {
                            for (d = 0; d < mchannels; d++)
                            {
                                fgsum[d] += (unsigned long int)pim[d];
                            }
                            fgnum++;
                        }
                        else
                        {
                            for (d = 0; d < mchannels; d++)
                            {
                                bgsum[d] += (unsigned long int)pim[d];
                            }
                            bgnum++;
                        }
                    }
                }
                if (fgnum > 0)
                {
                    for (d = 0; d < mchannels; d++)
                    {
                        fgsum[d] += (fgnum >> 1);
                        fgsum[d] /= fgnum;
                        fgim[d] = (int)fgsum[d];
                    }
                }
                else
                {
                    // mean region buffg
                    for (d = 0; d < mchannels; d++)
                    {
                        ims[d] = 0.0f;
                    }
                    n = 0;
                    k = 0;
                    for (y = y0b; y < y1b; y++)
                    {
                        for (x = x0b; x < x1b; x++)
                        {
                            k = (widthbg * y + x) * channels;
                            for (d = 0; d < mchannels; d++)
                            {
                                ims[d] += (float)buffg[k + d];
                            }
                            n++;
                        }
                    }
                    for (d = 0; d < mchannels; d++)
                    {
                        if (n > 0)
                        {
                            ims[d] /= (float)n;
                        }
                        ims[d] += 0.5f;
                        ims[d] = (ims[d] < 0.0f) ? 0.0f : (ims[d] < 255.0f) ? ims[d] : 255.0f;
                        fgim[d] = (int)ims[d];
                    }
                }
                if (bgnum > 0)
                {
                    for (d = 0; d < mchannels; d++)
                    {
                        bgsum[d] += (bgnum >> 1);
                        bgsum[d] /= bgnum;
                        bgim[d] = (int)bgsum[d];
                    }
                }
                else
                {
                    // mean region bufbg
                    for (d = 0; d < mchannels; d++)
                    {
                        ims[d] = 0.0f;
                    }
                    n = 0;
                    k = 0;
                    for (y = y0b; y < y1b; y++)
                    {
                        for (x = x0b; x < x1b; x++)
                        {
                            k = (widthbg * y + x) * channels;
                            for (d = 0; d < mchannels; d++)
                            {
                                ims[d] += (float)bufbg[k + d];
                            }
                            n++;
                        }
                    }
                    for (d = 0; d < mchannels; d++)
                    {
                        if (n > 0)
                        {
                            ims[d] /= (float)n;
                        }
                        ims[d] += 0.5f;
                        ims[d] = (ims[d] < 0.0f) ? 0.0f : (ims[d] < 255.0f) ? ims[d] : 255.0f;
                        bgim[d] = (int)ims[d];
                    }
                }

                // mean region buf
                for (d = 0; d < mchannels; d++)
                {
                    ims[d] = 0.0f;
                }
                n = 0;
                k = 0;
                for (y = y0; y < y1; y++)
                {
                    for (x = x0; x < x1; x++)
                    {
                        k = (width * y + x) * channels;
                        for (d = 0; d < mchannels; d++)
                        {
                            ims[d] += (float)buf[k + d];
                        }
                        n++;
                    }
                }
                for (d = 0; d < mchannels; d++)
                {
                    if (n > 0)
                    {
                        ims[d] /= (float)n;
                    }
                    if (ims[d] > 255.0f) ims[d] = 255.0f;
                    pim[d] = (int)(ims[d] + 0.5f);
                }

                // distance buffg -> buf, bufbg -> buf
                fgdist = 0.0f;
                for (d = 0; d < mchannels; d++)
                {
                    imd = pim[d];
                    imd -= fgim[d];
                    if (imd < 0) imd = -imd;
                    fgdist += imd;
                }
                bgdist = 0.0f;
                for (d = 0; d < mchannels; d++)
                {
                    imd = pim[d];
                    imd -= bgim[d];
                    if (imd < 0) imd = -imd;
                    bgdist += imd;
                }

                fgpart = 1.0f;
                bgpart = 1.0f;
                if ((fgdist + bgdist) > 0.0f)
                {
                    fgpart += (fgdist + fgdist) / (fgdist + bgdist);
                    bgpart += (bgdist + bgdist) / (fgdist + bgdist);
                }
                fgpart *= partl;
                bgpart *= partl;

                // average old and new FG
                parts = 1.0f /(fgpart + 1.0f);
                for (y = y0b; y < y1b; y++)
                {
                    for (x = x0b; x < x1b; x++)
                    {
                        k = (widthbg * y + x) * channels;
                        for (d = 0; d < mchannels; d++)
                        {
                            imx = (float)buffg[k + d];
                            imx *= fgpart;
                            imx += (float)fgim[d];
                            imx *= parts;
                            imx += 0.5f;
                            imx = (imx < 0.0f) ? 0.0f : (imx < 255.0f) ? imx : 255.0f;
                            buffg[k + d] = (unsigned char)imx;
                        }
                    }
                }

                // average old and new BG
                parts = 1.0f /(bgpart + 1.0f);
                for (y = y0b; y < y1b; y++)
                {
                    for (x = x0b; x < x1b; x++)
                    {
                        k = (widthbg * y + x) * channels;
                        for (d = 0; d < mchannels; d++)
                        {
                            imx = (float)bufbg[k + d];
                            imx *= bgpart;
                            imx += (float)bgim[d];
                            imx *= parts;
                            imx += 0.5f;
                            imx = (imx < 0.0f) ? 0.0f : (imx < 255.0f) ? imx : 255.0f;
                            bufbg[k + d] = (unsigned char)imx;
                        }
                    }
                }
            }
        }
        blsz >>= 1;
    }

    // threshold mask
    l = 0;
    lm = 0;
    for (y = 0; y < height; y++)
    {
        yb = y / bgs;
        for (x = 0; x < width; x++)
        {
            xb = x / bgs;
            k = (widthbg * yb + xb) * channels;
            for (d = 0; d < mchannels; d++)
            {
                pim[d] = (int)buf[l + d];
                fgim[d] = (int)buffg[k + d];
                bgim[d] = (int)bufbg[k + d];
            }
            l += channels;

            // distance buffg -> buf, bufbg -> buf
            fgdist = 0.0f;
            for (d = 0; d < mchannels; d++)
            {
                imd = pim[d];
                imd -= fgim[d];
                if (imd < 0) imd = -imd;
                fgdist += (float)imd;
            }
            bgdist = 0.0f;
            for (d = 0; d < mchannels; d++)
            {
                imd = pim[d];
                imd -= bgim[d];
                if (imd < 0) imd = -imd;
                bgdist += (float)imd;
            }
            bufmask[lm] = (fgdist < bgdist);
            lm++;
        }
    }

    return level;
}

DJVULAPI int ImageFGdownsample(unsigned char* buffg, unsigned int width, unsigned int height, unsigned int channels, unsigned int fgs)
{
    unsigned int widthfg, heightfg, y, x, y0, x0, y1, x1, xf, yf, d;
    unsigned long int s, n;
    size_t k, kf;

    if (fgs > 1)
    {
        widthfg = (width + fgs - 1) / fgs;
        heightfg = (height + fgs - 1) / fgs;
        k = 0;
        for (y = 0; y < heightfg; y++)
        {
            y0 = y * fgs;
            y1 = y0 + fgs;
            y1 = (y1 < height) ? y1 : height;
            for (x = 0; x < widthfg; x++)
            {
                x0 = x * fgs;
                x1 = x0 + fgs;
                x1 = (x1 < width) ? x1 : width;
                for (d = 0; d < channels; d++)
                {
                    s = 0;
                    n = 0;
                    for (yf = y0; yf < y1; yf++)
                    {
                        for (xf = x0; xf < x1; xf++)
                        {
                            kf = (width * yf + xf) * channels + d;
                            s += (unsigned long int)buffg[kf];
                            n++;
                        }
                    }
                    n = (n > 0) ? n : 1;
                    s += (n >> 1);
                    s /= n;
                    s = (s < 255) ? s : 255;
                    buffg[k] = (unsigned char)s;
                    k++;
                }
            }
        }
    }
    else
    {
        fgs = 1;
    }

    return fgs;
}

DJVULAPI int ImageDjvuReconstruct(unsigned char* buf, bool* bufmask, unsigned char* bufbg, unsigned char* buffg, unsigned int width, unsigned int height, unsigned int channels, unsigned int widthbg, unsigned int heightbg, unsigned int widthfg, unsigned int heightfg)
{
    unsigned int y, x, xbg, ybg, xfg, yfg, d, ground = 0;
    int bgsh, bgsw, fgsh, fgsw;
    int c, cp, cn, dx, dy, dcy, dcx;
    unsigned char bgc, fgc;
    size_t k, km, kbg, kfg, linebg, linefg;

    if (bufbg)
    {
        bgsh = (height + heightbg - 1) / heightbg;
        bgsw = (width + widthbg - 1) / widthbg;
        ground++;
    }
    else
    {
        bgsh = 1;
        bgsw = 1;
    }
    if (buffg)
    {
        fgsh = (height + heightfg - 1) / heightfg;
        fgsw = (width + widthfg - 1) / widthfg;
        ground++;
    }
    else
    {
        fgsh = 1;
        fgsw = 1;
    }
    linebg = widthbg * channels;
    linefg = widthfg * channels;
    k = 0;
    km = 0;
    for (y = 0; y < height; y++)
    {
        ybg = y / bgsh;
        yfg = y / fgsh;
        for (x = 0; x < width; x++)
        {
            xbg = x / bgsw;
            xfg = x / fgsw;
            for (d = 0; d < channels; d++)
            {
                if (bufbg)
                {
                    kbg = (widthbg * ybg + xbg) * channels + d;
                    c = (int)bufbg[kbg];

                    cp = (ybg > 0) ? (int)bufbg[kbg - linebg] : c;
                    cn = (ybg < heightbg - 1) ? (int)bufbg[kbg + linebg] : c;
                    dcy = cn - cp;
                    dy = y - ybg * bgsh;
                    cp = (xbg > 0) ? (int)bufbg[kbg - channels] : c;
                    cn = (xbg < widthbg - 1) ? (int)bufbg[kbg + channels] : c;
                    dcx = cn - cp;
                    dx = x - xbg * bgsw;
                    c += (dcy * (2 * dy + 1 - bgsh) / bgsh + dcx * (2 * dx + 1 - bgsw) / bgsw) / 4;

                    bgc = (unsigned char)((c < 0) ? 0 : (c < 255) ? c : 255);
                }
                else
                {
                    bgc = 255;
                }
                if (buffg)
                {
                    kfg = (widthfg * yfg + xfg) * channels + d;
                    c = (int)buffg[kfg];

                    cp = (yfg > 0) ? (int)buffg[kfg - linefg] : c;
                    cn = (yfg < heightfg - 1) ? (int)buffg[kfg + linefg] : c;
                    dcy = cn - cp;
                    dy = y - yfg * fgsh;
                    cp = (xfg > 0) ? (int)buffg[kfg - channels] : c;
                    cn = (xfg < widthfg - 1) ? (int)buffg[kfg + channels] : c;
                    dcx = cn - cp;
                    dx = x - xfg * fgsw;
                    c += (dcy * (2 * dy + 1 - fgsh) / fgsh + dcx * (2 * dx + 1 - fgsw) / fgsw) / 4;

                    fgc = (unsigned char)((c < 0) ? 0 : (c < 255) ? c : 255);
                }
                else
                {
                    fgc = 0;
                }
                buf[k] = (bufmask[km]) ? fgc : bgc;
                k++;
            }
            km++;
        }
    }

    return ground;
}

DJVULAPI int ImageDjvulSelect(unsigned char* buf, bool* bufmask, unsigned char* bufbg, unsigned char* buffg, unsigned int width, unsigned int height, unsigned int channels, unsigned int bgs, unsigned int level, int wbmode, float doverlay, float anisotropic, float contrast, float fbscale, float delta, float radius, float sensitivity, int method)
{
    switch(method)
    {
    case TBIMOD:
        (void)ImageThresholdBimod(buf, bufmask, width, height, channels, fbscale, delta);
        return ImageDjvulGround(buf, bufmask, bufbg, buffg, width, height, channels, bgs, level, doverlay);
        break;
    case TSAUVOLA:
        (void)ImageThresholdSauvola(buf, bufmask, width, height, channels, (int)radius, sensitivity, fbscale, 0, 255, delta);
        return ImageDjvulGround(buf, bufmask, bufbg, buffg, width, height, channels, bgs, level, doverlay);
        break;
    case TBLUR:
        (void)ImageThresholdBlur(buf, bufmask, width, height, channels, radius, fbscale, delta, sensitivity);
        return ImageDjvulGround(buf, bufmask, bufbg, buffg, width, height, channels, bgs, level, doverlay);
        break;
    case TEDGEPLUS:
        (void)ImageThresholdEdgePlus(buf, bufmask, width, height, channels, radius, fbscale, delta, sensitivity);
        return ImageDjvulGround(buf, bufmask, bufbg, buffg, width, height, channels, bgs, level, doverlay);
        break;
    default:
        return ImageDjvulThreshold(buf, bufmask, bufbg, buffg, width, height, channels, bgs, level, wbmode, doverlay, anisotropic, contrast, fbscale, delta);
        break;
    }
}

#endif /* DJVUL_IMPLEMENTATION */

#endif /* DJVUL_H_ */
