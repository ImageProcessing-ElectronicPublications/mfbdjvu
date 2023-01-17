# MFBdjvu

MFBdjvu is a simple project for easy converting pgm and ppm to (MASK+FG+BG)-djvu.
It uses [djvulibre](http://djvu.sourceforge.net/) for all technichal work and compression.
The breakdown of the image into components is done using [DjVuL](https://github.com/plzombie/depress/issues/2) and [DjVuL wiki](https://sourceforge.net/p/imthreshold/wiki/DjVuL/?version=3).

MFBdjvu based of [simpledjvu](https://github.com/mihaild/simpledjvu).

## Install

### load submodules

submodules:

- [djvulibre](https://github.com/barak/djvulibre) -> [src](src)

```shell
$ git submodule init
$ git submodule update
```

### build

Typo:
```shell
$ make
```

If you want, you can change your PATH variable or copy **mfbdjvu** binary to any directory already included in your PATH.

You need `g++` version supports `c++0x` standard flag.

## Usage

```shell
mfbdjvu [options] input.pnm output.djvu
```

where options =

**-dpi n** {300} DPI output djvu.

**-loss n** {1} Use *n* as cjb2 loss level (see djvulibre cjb2 tool description).

**-slices_bg n1,n2,...** {74,89,89}. Use *n1,n2,...* as number of slices for c44 for background (see djvulibre c44 tool description).

**-slices_fg n1,n2,...** {89} Use *n1,n2,...* as number of slices for c44 for foreground.

**-levels n**  {0} Level DjVuL block, 0 - auto.

**-bgs n** {3} Background and Foreground downsample.

**-fgs n** {2} Foreground more downsample.

**-overlay n** {50} Block overlay DjVuL in percent.

**-anisotropic n** {0} The main regulator DjVuL in percent. More than zero - more details, less than zero - less details.

**-contrast n** {0} Auxiliary regulator DjVuL in percent (sharpen).

**-fbs n** {100} and **-delta n** {0} Additional regulation DjVuL of BG/FG according to the linear law: FG * fbs + delta != BG

**-black** Using black BG as the base, not white.

You can use [Netpbm](https://sourceforge.net/projects/netpbm/) or any other similar tool to obtain `pgm` or `ppm` from other format.

## DjVuL description.

The [base of the algorithm](https://sourceforge.net/p/imthreshold/wiki/DjVuL/?version=3) was obtained in 2016 by studying the works [monday2000](http://djvu-soft.narod.ru/) and adapting them to Linux.
The prerequisite was the [BookScanLib](http://djvu-soft.narod.ru/bookscanlib/) project  and the algorithm [DjVu Thresholding Binarization](http://djvu-soft.narod.ru/bookscanlib/034.htm).
This algorithm embodied good ideas, but had a recursive structure, was a "function with discontinuities" and had a hard color limit.
The result of this algorithm, due to the indicated shortcomings and the absence of regulators, was doubtful.
After careful study, all the foundations of the specified algorithm were rejected.
The new algorithm is based on levels instead of recursion, a smooth weight function is used instead of a "discontinuous" one, no color restriction, BG/FG selection controls are enabled.
The new algorithm allowed not only to obtain a much more adequate result, but also gave derivative functions: image division into BG/FG according to the existing mask.
