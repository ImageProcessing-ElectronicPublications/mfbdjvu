/*
 * MFBdjvu-1.6
 * Based on simpledjvu, djvul and djvulibre (http://djvu.sourceforge.net/)
 *
 */

#define MFBDJVU_VERSION "1.6"

#include <iostream>
#include <cstdlib>
#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <djvulibre.h>
#include <pgm2jb2.h>
#define DJVUL_IMPLEMENTATION
#include "djvul.h"

using std::vector;
using std::array;
using std::string;

struct Keys
{
    int dpi;
    int loss;
    float quality;
    int levels;
    int bgs;
    int fgs;
    int wbmode;
    float overlay;
    float anisotropic;
    float contrast;
    float fbs;
    float delta;
    vector<int> slices_bg;
    vector<int> slices_fg;
    char *mask;
    Keys():
        dpi(300),
        loss(1),
        quality(0.75f),
        levels(0),
        bgs(3),
        fgs(2),
        wbmode(1),
        overlay(0.5f),
        anisotropic(0.0f),
        contrast(0.0f),
        fbs(1.0f),
        delta(0.0f),
        slices_bg({74,84,88,97}),
        slices_fg({100}),
        mask(NULL)
    {
    }

};

void save(const GP<GBitmap> gimage, const char *fname, bool pgm = true)
{
    if (pgm)
    {
        gimage->save_pgm(*ByteStream::create(GURL::Filename::UTF8(fname), "wb"));
    }
    else
    {
        gimage->save_pbm(*ByteStream::create(GURL::Filename::UTF8(fname), "wb"));
    }
}

enum Chunk { BACKGROUND, FOREGROUND};

GP<GBitmap> make_chunk_mask(const GBitmap &mask, Chunk chunk)
{
    GP<GBitmap> result = GBitmap::create(mask.rows(), mask.columns());
    result->set_grays(2);
    GBitmap &raw_result = *result;
    int ok_color = chunk == BACKGROUND;
    for (int i = 0; i < mask.rows(); ++i)
    {
        for (int j = 0; j < mask.columns(); ++j)
        {
            raw_result[i][j] = (mask[i][j] == ok_color);
        }
    }
    return result;
}

void print_help()
{
    std::cerr
            << "MFB DjVu version " << MFBDJVU_VERSION << "\n"
            << "Usage: mfbdjvu [options] input.pnm output.djvu\n"
            << "where options =\n"
            << "\t-mask mask.pbm\n"
            << "\t-dpi n {300}\n"
            << "\t-loss n {1}\n"
            << "\t-quality n {75}\n"
            << "\t-slices_bg n1,n2,.. {74,84,88,97}\n"
            << "\t-slices_fg n1,n2,...{100}\n"
            << "\t-levels n {0}\n"
            << "\t-bgs n {3}\n"
            << "\t-fgs n {2}\n"
            << "\t-overlay n {50}\n"
            << "\t-anisotropic n {0}\n"
            << "\t-contrast n {0}\n"
            << "\t-fbs n {100}\n"
            << "\t-delta n {0}\n"
            << "\t-black\n"
            ;
}

bool parse_keys(int argc, char *argv[], Keys *keys, char **input, char **output)
{
    if (argc <= 2)
    {
        std::cerr << "Not enough arguments\n";
        return false;
    }
    (*input) = argv[argc - 2];
    (*output) = argv[argc - 1];
    for (int i = 1; i < argc - 2; ++i)
    {
        if (argv[i][0] != '-')
        {
            std::cerr << "Wrong option format: " << argv[i] << '\n';
            return false;
        }
        string arg = argv[i];
        if (arg == "-black")
        {
            keys->wbmode = -1;
        }
        else if (arg == "-dpi" || arg == "-loss" || arg == "-quality" || arg == "-levels" || arg == "-bgs" || arg == "-fgs" || arg == "-overlay" || arg == "-anisotropic" || arg == "-contrast" || arg == "-fbs" || arg == "-delta")
        {
            ++i;
            int n;
            char *endptr;
            n = strtol(argv[i], &endptr, 10);
            if (*endptr)
            {
                std::cerr << "Bad number: " << argv[i] << '\n';
                return false;
            }
            if (arg == "-dpi")
            {
                keys->dpi = n;
            }
            else if (arg == "-loss")
            {
                keys->loss = n;
            }
            else if (arg == "-quality")
            {
                n = (n < 0) ? 0 : n;
                keys->quality = (float)n * 0.01f;
                float lq = log10(1.0f + keys->quality * 12.0f); // == 1.0 for quality 75
                vector<int> nf, nb;
                nb.push_back((int)(lq * 57.0f + 0.5f) + 17);
                nb.push_back((int)(lq * 66.0f + 0.5f) + 18);
                nb.push_back((int)(lq * 69.0f + 0.5f) + 19);
                nb.push_back((int)(lq * 77.0f + 0.5f) + 20);
                keys->slices_bg = nb;
                nf.push_back((int)(lq * 80.0f + 0.5f) + 20);
                keys->slices_fg = nf;
            }
            else if (arg == "-levels")
            {
                keys->levels = n;
            }
            else if (arg == "-bgs")
            {
                keys->bgs = n;
            }
            else if (arg == "-fgs")
            {
                keys->fgs = n;
            }
            else if (arg == "-overlay")
            {
                keys->overlay = (float)n * 0.01f;
            }
            else if (arg == "-anisotropic")
            {
                keys->anisotropic = (float)n * 0.01f;
            }
            else if (arg == "-contrast")
            {
                keys->contrast = (float)n * 0.01f;
            }
            else if (arg == "-fbs")
            {
                keys->fbs = (float)n * 0.01f;
            }
            else if (arg == "-delta")
            {
                keys->delta = (float)n;
            }
        }
        else if (arg == "-slices_bg" || arg == "-slices_fg")
        {
            ++i;
            char *nptr = argv[i], *endptr;
            vector<int> ns;
            while (*nptr)
            {
                int n = strtol(nptr, &endptr, 10);
                if (endptr == nptr || *endptr != ',' && *endptr != 0)
                {
                    std::cerr << "Bad numbers: " << argv[i] << '\n';
                    return false;
                }
                ns.push_back(n);
                nptr = *endptr ? endptr + 1 : endptr;
            }
            if (arg == "-slices_bg")
            {
                keys->slices_bg = ns;
            }
            else if (arg == "-slices_fg")
            {
                keys->slices_fg = ns;
            }
        }
        else if (arg == "-mask")
        {
            ++i;
            char *nptr = argv[i];
            keys->mask = nptr;
        }
        else
        {
            std::cerr << "Unknown option: " << argv[i] << '\n';
            return false;
        }
    }
    return true;
}

/*
 * random parts from c44 tool source code
 * random mix of references and pointers, just like in main djvulibre code
 *
 * @todo: understand, how does it work
 */
void write_part_to_djvu(const GPixmap &image, const vector<int> &slices, const GP<GBitmap> &gmask, IFFByteStream &iff, Chunk chunk)
{
    GP<IW44Image> iw = IW44Image::create_encode(image, gmask, IW44Image::CRCBnormal);
    vector<IWEncoderParms> parms(slices.size());
    for (int i = 0; i < parms.size(); ++i)
    {
        parms[i].slices = slices[i];
        // is it necessary?
        parms[i].bytes = 0;
        parms[i].decibels = 0;
    }

    for (const auto& parm : parms)
    {
        if (chunk == BACKGROUND)
        {
            iff.put_chunk("BG44");
        }
        else
        {
            iff.put_chunk("FG44");
        }
        iw->encode_chunk(iff.get_bytestream(), parm);
        iff.close_chunk();
    }
}

int main(int argc, char *argv[])
{
    int height, width, heightm, widthm, channels;
    unsigned char *data = NULL;
    bool *mask_data = NULL;
    unsigned char *bg_data = NULL;
    unsigned char *fg_data = NULL;
    int bg_width, bg_height, fg_width, fg_height;
    int y, x, d;
    size_t kd, ki;
    Keys keys;
    char *input, *output;
    if (!parse_keys(argc, argv, &keys, &input, &output))
    {
        print_help();
        return 1;
    }
    if (keys.mask)
    {
        GP<GBitmap> bimage;
        GP<ByteStream> gibs = ByteStream::create(GURL::Filename::UTF8(keys.mask), "rb");
        ByteStream &ibs = *gibs;
        char prefix[16];
        memset(prefix, 0, sizeof(prefix));
        if (ibs.readall((void*)prefix, sizeof(prefix)) < sizeof(prefix))
        {
            G_THROW(ERR_MSG("mfbdjvu.failed_pnm_header"));
            return 1;
        }
        ibs.seek(0);

        if (prefix[0]=='P' && (prefix[1]=='1' || prefix[1]=='4'))
        {
            // bw file
            bimage = GBitmap::create(ibs);
            GBitmap &raw_bimage=*bimage;
            heightm = bimage->rows();
            widthm = bimage->columns();
            channels = 1;

            if (!(mask_data = (bool*)malloc(heightm * widthm * sizeof(bool))))
            {
                fprintf(stderr, "ERROR: not memiory\n");
                return 2;
            }
            kd = 0;
            for (int y = 0; y < heightm; y++)
            {
                for (int x = 0; x < widthm; x++)
                {
                    mask_data[kd] = (raw_bimage[y][x] > 0);
                    kd++;
                }
            }
        }
        else
        {
            keys.mask = NULL;
        }
    }

    GP<ByteStream> gibs = ByteStream::create(GURL::Filename::UTF8(input), "rb");
    ByteStream &ibs = *gibs;
    char prefix[16];
    memset(prefix, 0, sizeof(prefix));
    if (ibs.readall((void*)prefix, sizeof(prefix)) < sizeof(prefix))
    {
        G_THROW(ERR_MSG("mfbdjvu.failed_pnm_header"));
        return 1;
    }
    ibs.seek(0);

    if (prefix[0]=='P' && (prefix[1]=='2' || prefix[1]=='5'))
    {
        // gray file
        GP<GBitmap> gimage = GBitmap::create(ibs);
        GBitmap &raw_image=*gimage;
        height = gimage->rows();
        width = gimage->columns();
        channels = 1;
        if (keys.mask)
        {
            if ((heightm != height) || (widthm != width))
            {
                fprintf(stderr, "ERROR: bad mask size\n");
                return 2;
            }
        }

        if (!(data = (unsigned char*)malloc(height * width * channels * sizeof(unsigned char))))
        {
            fprintf(stderr, "ERROR: not use memmory\n");
            return 1;
        }

        kd = 0;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                data[kd] = (unsigned char)(255 - raw_image[y][x]);
                kd++;
            }
        }
    }
    else if (prefix[0]=='P' && (prefix[1]=='3' || prefix[1]=='6'))
    {
        GP<GPixmap> gimage=GPixmap::create(ibs);
        GPixmap &raw_image=*gimage;
        height = gimage->rows();
        width = gimage->columns();
        channels = 3;
        if (keys.mask)
        {
            if ((heightm != height) || (widthm != width))
            {
                fprintf(stderr, "ERROR: bad mask size\n");
                return 2;
            }
        }

        if (!(data = (unsigned char*)malloc(height * width * channels * sizeof(unsigned char))))
        {
            fprintf(stderr, "ERROR: not use memmory\n");
            return 1;
        }

        kd = 0;
        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                GPixel p = raw_image[y][x];
                data[kd] = (unsigned char)p.r;
                data[kd + 1] = (unsigned char)p.g;
                data[kd + 2] = (unsigned char)p.b;
                kd += channels;
            }
        }
    }
    else
    {
        G_THROW(ERR_MSG("mfbdjvu.unknow_file"));
        return 1;
    }

    bg_width = (width + keys.bgs - 1) / keys.bgs;
    bg_height = (height + keys.bgs - 1) / keys.bgs;
    fg_width = (bg_width + keys.fgs - 1) / keys.fgs;
    fg_height = (bg_height + keys.fgs - 1) / keys.fgs;

    if (!keys.mask)
    {
        if (!(mask_data = (bool*)malloc(height * width * sizeof(bool))))
        {
            fprintf(stderr, "ERROR: not memiory\n");
            return 2;
        }
    }
    if (!(bg_data = (unsigned char*)malloc(bg_height * bg_width * channels * sizeof(unsigned char))))
    {
        fprintf(stderr, "ERROR: not memiory\n");
        return 2;
    }
    if (!(fg_data = (unsigned char*)malloc(bg_height * bg_width * channels * sizeof(unsigned char))))
    {
        fprintf(stderr, "ERROR: not memiory\n");
        return 2;
    }

    if (keys.mask)
    {
        if(!(keys.levels = ImageDjvulGround(data, mask_data, bg_data, fg_data, width, height, channels, keys.bgs, keys.levels, keys.overlay)))
        {
            fprintf(stderr, "ERROR: not complite DjVuL ground\n");
            return 3;
        }
    }
    else
    {
        if(!(keys.levels = ImageDjvulThreshold(data, mask_data, bg_data, fg_data, width, height, channels, keys.bgs, keys.levels, keys.wbmode, keys.overlay, keys.anisotropic, keys.contrast, keys.fbs, keys.delta)))
        {
            fprintf(stderr, "ERROR: not complite DjVuL\n");
            return 3;
        }
    }

    if (keys.fgs > 1)
    {
        keys.fgs = ImageFGdownsample(fg_data, bg_width, bg_height, channels, keys.fgs);
    }
    GP<GBitmap> gnormalized = GBitmap::create(height, width);
    GBitmap &raw_gnorm=*gnormalized;
    kd = 0;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            raw_gnorm[y][x] = (unsigned char)((mask_data[kd]) ? 255 : 0);
            kd++;
        }
    }

    GP<JB2Image> gmask = pbm2jb2(gnormalized, keys.loss);

    /*
     * this code is based on djvumake and c44 tools source
     */
    GP<IFFByteStream> giff = IFFByteStream::create(ByteStream::create(GURL::Filename::UTF8(output), "wb"));
    IFFByteStream &iff = *giff;
    iff.put_chunk("FORM:DJVU", 1);

    GP<DjVuInfo> ginfo = DjVuInfo::create();
    ginfo->width = gmask->get_width();
    ginfo->height = gmask->get_height();
    ginfo->dpi = keys.dpi;

    iff.put_chunk("INFO");
    ginfo->encode(*iff.get_bytestream());
    iff.close_chunk();

    iff.put_chunk("Sjbz");
    gmask->encode(iff.get_bytestream());
    iff.close_chunk();

    GP<GPixmap> bgimage = GPixmap::create(bg_height, bg_width);
    GPixmap &raw_bg=*bgimage;
    GP<GBitmap> bgmask = GBitmap::create(bg_height, bg_width);
    GBitmap &raw_bgm=*bgmask;
    GP<GPixmap> fgimage = GPixmap::create(fg_height, fg_width);
    GPixmap &raw_fg=*fgimage;
    GP<GBitmap> fgmask = GBitmap::create(fg_height, fg_width);
    GBitmap &raw_fgm=*fgmask;
    if (channels == 1)
    {
        kd = 0;
        for (y = 0; y < bg_height; y++)
        {
            for (x = 0; x < bg_width; x++)
            {
                raw_bg[y][x].r = (unsigned char)bg_data[kd];
                raw_bg[y][x].g = (unsigned char)bg_data[kd];
                raw_bg[y][x].b = (unsigned char)bg_data[kd];
                kd++;
            }
        }
        for (y = 0; y < bg_height; y++)
        {
            for (x = 0; x < bg_width; x++)
            {
                raw_bgm[y][x] = 255;
            }
        }
        kd = 0;
        for (y = 0; y < height; y++)
        {
            int yb = y / keys.bgs;
            for (x = 0; x < width; x++)
            {
                int xb = x / keys.bgs;
                if (!mask_data[kd])
                {
                    raw_bgm[yb][xb] = 0;
                }
                kd++;
            }
        }
        kd = 0;
        for (y = 0; y < fg_height; y++)
        {
            for (x = 0; x < fg_width; x++)
            {
                raw_fg[y][x].r = (unsigned char)fg_data[kd];
                raw_fg[y][x].g = (unsigned char)fg_data[kd];
                raw_fg[y][x].b = (unsigned char)fg_data[kd];
                kd++;
            }
        }
        for (y = 0; y < fg_height; y++)
        {
            for (x = 0; x < fg_width; x++)
            {
                raw_fgm[y][x] = 0;
            }
        }
        kd = 0;
        for (y = 0; y < height; y++)
        {
            int yf = y / keys.bgs / keys.fgs;
            for (x = 0; x < width; x++)
            {
                int xf = x / keys.bgs / keys.fgs;
                if (mask_data[kd])
                {
                    raw_fgm[yf][xf] = 255;
                }
                kd++;
            }
        }
    }
    else
    {
        kd = 0;
        for (y = 0; y < bg_height; y++)
        {
            for (x = 0; x < bg_width; x++)
            {
                GPixel p;
                p.r = (unsigned char)bg_data[kd];
                p.g = (unsigned char)bg_data[kd + 1];
                p.b = (unsigned char)bg_data[kd + 2];
                raw_bg[y][x] = p;
                kd += channels;
            }
        }
        for (y = 0; y < bg_height; y++)
        {
            for (x = 0; x < bg_width; x++)
            {
                raw_bgm[y][x] = 255;
            }
        }
        kd = 0;
        for (y = 0; y < height; y++)
        {
            int yb = y / keys.bgs;
            for (x = 0; x < width; x++)
            {
                int xb = x / keys.bgs;
                if (!mask_data[kd])
                {
                    raw_bgm[yb][xb] = 0;
                }
                kd++;
            }
        }
        kd = 0;
        for (y = 0; y < fg_height; y++)
        {
            for (x = 0; x < fg_width; x++)
            {
                GPixel p;
                p.r = (unsigned char)fg_data[kd];
                p.g = (unsigned char)fg_data[kd + 1];
                p.b = (unsigned char)fg_data[kd + 2];
                raw_fg[y][x] = p;
                kd += channels;
            }
        }
        for (y = 0; y < fg_height; y++)
        {
            for (x = 0; x < fg_width; x++)
            {
                raw_fgm[y][x] = 0;
            }
        }
        kd = 0;
        for (y = 0; y < height; y++)
        {
            int yf = y / keys.bgs / keys.fgs;
            for (x = 0; x < width; x++)
            {
                int xf = x / keys.bgs / keys.fgs;
                if (mask_data[kd])
                {
                    raw_fgm[yf][xf] = 255;
                }
                kd++;
            }
        }
    }
    bgmask->binarize_grays(127);
    fgmask->binarize_grays(127);

    write_part_to_djvu(*fgimage, keys.slices_fg, make_chunk_mask(*fgmask, FOREGROUND), iff, FOREGROUND);
    write_part_to_djvu(*bgimage, keys.slices_bg, make_chunk_mask(*bgmask, BACKGROUND), iff, BACKGROUND);

    return 0;
}
