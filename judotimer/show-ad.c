/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright 2006-2023 Hannu Jokinen
 * 
 * This file is part of JudoShiai.
 *
 * JudoShiai is free software: you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * JudoShiai is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with JudoShiai. If not, see
 * <https://www.gnu.org/licenses/>.
 */

/* GIF-decoding functions are extracted from Gif-Lib
   by Gershon Elber, Eric S. Raymond, Toshio Kuratomi.

   The GIFLIB distribution is Copyright (c) 1997  Eric S. Raymond

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

/* The creators of the GIF format require the following
   acknowledgement:
   The Graphics Interchange Format(c) is the Copyright property of
   CompuServe Incorporated. GIF(sm) is a Service Mark property of
   CompuServe Incorporated.
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <librsvg/rsvg.h>
#include <gdk/gdkkeysyms-compat.h>

#include "judotimer.h"
#include "common-utils.h"
#include "svg-common.h"
#include "cJSON.h"

/**** SVG RELATED ****/

enum {
    IMAGE_FLAG1,
    IMAGE_FLAG2,
    IMAGE_CLOSE,
    IMAGE_CONTROL1,
    IMAGE_CONTROL2,
    IMAGE_PREVFLAG1,
    IMAGE_PREVFLAG2,
    IMAGE_NEXTFLAG1,
    IMAGE_NEXTFLAG2,
    NUM_IMAGES
};

const gchar *imagename[NUM_SVG_IMAGES] = {
    "flag1", "flag2", "close", "control1", "control2", "prevflag1", "prevflag2", "nextflag1", "nextflag2", NULL
};

static svg_handle ad_svg_handle = {0};
static struct svg_memory svg_reply = {0};
static gchar *prevteam1name, *prevteam2name;
static gchar *nextteam1name, *nextteam2name;
static gchar *prevteam1country, *prevteam2country;
static gchar *nextteam1country, *nextteam2country;
static gint team1wins, team2wins;
static gint team1pts, team2pts;


/********/

void set_svg_file(GtkWidget *menu_item, GdkEventButton *event, gpointer data);
gint paint_svg(struct paint_data *pd);
static gboolean mouse_click(GtkWidget *window, GdkEventButton *event, gpointer userdata);
static void clear_data(void);

gint adwin_x = 0, adwin_y = 0, adwin_w = 0, adwin_h = 0;
gboolean showflags = FALSE, showletter = FALSE;
gdouble  flagsize = 7.0, namesize = 10.0;

//#define T mylog("Error on line %d\n", __LINE__)
#define T do {} while (0)

#define UINT32 uint32_t

#define HT_SIZE			8192	   /* 12bits = 4096 or twice as big! */
#define HT_KEY_MASK		0x1FFF			      /* 13bits keys */
#define HT_KEY_NUM_BITS		13			      /* 13bits keys */
#define HT_MAX_KEY		8191	/* 13bits - 1, maximal code possible */
#define HT_MAX_CODE		4095	/* Biggest code possible in 12 bits. */

#define HT_GET_KEY(l)	(l >> 12)
#define HT_GET_CODE(l)	(l & 0x0FFF)
#define HT_PUT_KEY(l)	(l << 12)
#define HT_PUT_CODE(l)	(l & 0x0FFF)

typedef struct GifHashTableType {
    UINT32 HTable[HT_SIZE];
} GifHashTableType;

#define GIF_ERROR   0
#define GIF_OK      1

#ifndef TRUE
#define TRUE        1
#endif /* TRUE */
#ifndef FALSE
#define FALSE       0
#endif /* FALSE */

#ifndef NULL
#define NULL        0
#endif /* NULL */

#define GIF_STAMP "GIFVER"          /* First chars in file - GIF stamp.  */
#define GIF_STAMP_LEN sizeof(GIF_STAMP) - 1
#define GIF_VERSION_POS 3           /* Version first character in stamp. */
#define GIF87_STAMP "GIF87a"        /* First chars in file - GIF stamp.  */
#define GIF89_STAMP "GIF89a"        /* First chars in file - GIF stamp.  */

#define GIF_FILE_BUFFER_SIZE 16384  /* Files uses bigger buffers than usual. */

typedef int GifBooleanType;
typedef unsigned char GifPixelType;
typedef unsigned char *GifRowType;
typedef unsigned char GifByteType;
typedef unsigned int GifPrefixType;
typedef int GifWord;


#define GIF_MESSAGE(Msg) fprintf(stderr, "\n%s\n", Msg)
#define GIF_EXIT(Msg)    { GIF_MESSAGE(Msg); return(-3); }

#define VoidPtr void *

typedef struct GifColorType {
    GifByteType Red, Green, Blue;
} GifColorType;

typedef struct ColorMapObject {
    int ColorCount;
    int BitsPerPixel;
    GifColorType *Colors;    /* on malloc(3) heap */
} ColorMapObject;

typedef struct GifImageDesc {
    GifWord Left, Top, Width, Height,   /* Current image dimensions. */
        Interlace;                    /* Sequential/Interlaced lines. */
    ColorMapObject *ColorMap;       /* The local color map */
} GifImageDesc;

typedef struct GifFileType {
    GifWord SWidth, SHeight,        /* Screen dimensions. */
        SColorResolution,         /* How many colors can we generate? */
        SBackGroundColor;         /* I hope you understand this one... */
    ColorMapObject *SColorMap;  /* NULL if not exists. */
    int ImageCount;             /* Number of current image */
    GifImageDesc Image;         /* Block describing current image */
    struct SavedImage *SavedImages; /* Use this to accumulate file state */
    VoidPtr UserData;           /* hook to attach user data (TVT) */
    VoidPtr Private;            /* Don't mess with this! */
} GifFileType;

typedef enum {
    UNDEFINED_RECORD_TYPE,
    SCREEN_DESC_RECORD_TYPE,
    IMAGE_DESC_RECORD_TYPE, /* Begin with ',' */
    EXTENSION_RECORD_TYPE,  /* Begin with '!' */
    TERMINATE_RECORD_TYPE   /* Begin with ';' */
} GifRecordType;

typedef enum {
    GIF_DUMP_SGI_WINDOW = 1000,
    GIF_DUMP_X_WINDOW = 1001
} GifScreenDumpType;

/* func type to read gif data from arbitrary sources (TVT) */
typedef int (*InputFunc) (GifFileType *, GifByteType *, int);

/* func type to write gif data ro arbitrary targets.
 * Returns count of bytes written. (MRB)
 */
typedef int (*OutputFunc) (GifFileType *, const GifByteType *, int);

#define COMMENT_EXT_FUNC_CODE     0xfe    /* comment */
#define GRAPHICS_EXT_FUNC_CODE    0xf9    /* graphics control */
#define PLAINTEXT_EXT_FUNC_CODE   0x01    /* plaintext */
#define APPLICATION_EXT_FUNC_CODE 0xff    /* application block */

#define E_GIF_ERR_OPEN_FAILED    1    /* And EGif possible errors. */
#define E_GIF_ERR_WRITE_FAILED   2
#define E_GIF_ERR_HAS_SCRN_DSCR  3
#define E_GIF_ERR_HAS_IMAG_DSCR  4
#define E_GIF_ERR_NO_COLOR_MAP   5
#define E_GIF_ERR_DATA_TOO_BIG   6
#define E_GIF_ERR_NOT_ENOUGH_MEM 7
#define E_GIF_ERR_DISK_IS_FULL   8
#define E_GIF_ERR_CLOSE_FAILED   9
#define E_GIF_ERR_NOT_WRITEABLE  10

static GifFileType *DGifOpenFileName(const char *GifFileName);
static GifFileType *DGifOpenFileHandle(int GifFileHandle);

static int DGifGetScreenDesc(GifFileType * GifFile);
static int DGifGetRecordType(GifFileType * GifFile, GifRecordType * GifType);
static int DGifGetImageDesc(GifFileType * GifFile);
static int DGifGetLine(GifFileType * GifFile, GifPixelType * GifLine, int GifLineLen);
static int DGifGetExtension(GifFileType * GifFile, int *GifExtCode,
                            GifByteType ** GifExtension);
static int DGifGetExtensionNext(GifFileType * GifFile, GifByteType ** GifExtension);
static int DGifGetCodeNext(GifFileType * GifFile, GifByteType ** GifCodeBlock);
static int DGifCloseFile(GifFileType * GifFile);
static int DGifGetScreenDesc(GifFileType * GifFile);

#define D_GIF_ERR_OPEN_FAILED    101    /* And DGif possible errors. */
#define D_GIF_ERR_READ_FAILED    102
#define D_GIF_ERR_NOT_GIF_FILE   103
#define D_GIF_ERR_NO_SCRN_DSCR   104
#define D_GIF_ERR_NO_IMAG_DSCR   105
#define D_GIF_ERR_NO_COLOR_MAP   106
#define D_GIF_ERR_WRONG_RECORD   107
#define D_GIF_ERR_DATA_TOO_BIG   108
#define D_GIF_ERR_NOT_ENOUGH_MEM 109
#define D_GIF_ERR_CLOSE_FAILED   110
#define D_GIF_ERR_NOT_READABLE   111
#define D_GIF_ERR_IMAGE_DEFECT   112
#define D_GIF_ERR_EOF_TOO_SOON   113

/******************************************************************************
 * O.K., here are the routines from GIF_LIB file GIF_ERR.C.
 ******************************************************************************/
static void PrintGifError1(int line);

/******************************************************************************
 * Color Map handling from ALLOCGIF.C
 *****************************************************************************/

static ColorMapObject *MakeMapObject(int ColorCount,
                                     const GifColorType * ColorMap);
static void FreeMapObject(ColorMapObject * Object);
static int BitSize(int n);

/******************************************************************************
 * Support for the in-core structures allocation (slurp mode).
 *****************************************************************************/

/* This is the in-core version of an extension record */
typedef struct {
    int ByteCount;
    char *Bytes;    /* on malloc(3) heap */
    int Function;   /* Holds the type of the Extension block. */
} ExtensionBlock;

/* This holds an image header, its unpacked raster bits, and extensions */
typedef struct SavedImage {
    GifImageDesc ImageDesc;
    unsigned char *RasterBits;  /* on malloc(3) heap */
    int Function;   /* DEPRECATED: Use ExtensionBlocks[x].Function instead */
    int ExtensionBlockCount;
    ExtensionBlock *ExtensionBlocks;    /* on malloc(3) heap */
} SavedImage;

static void FreeExtension(SavedImage * Image);
static void FreeSavedImages(GifFileType * GifFile);

/******************************************************************************
 * The library's internal utility font
 *****************************************************************************/

#define GIF_FONT_WIDTH  8
#define GIF_FONT_HEIGHT 8
//unsigned char AsciiTable[][GIF_FONT_WIDTH];

#define LZ_MAX_CODE         4095    /* Biggest code possible in 12 bits. */
#define LZ_BITS             12

#define FLUSH_OUTPUT        4096    /* Impossible code, to signal flush. */
#define FIRST_CODE          4097    /* Impossible code, to signal first. */
#define NO_SUCH_CODE        4098    /* Impossible code, to signal empty. */

#define FILE_STATE_WRITE    0x01
#define FILE_STATE_SCREEN   0x02
#define FILE_STATE_IMAGE    0x04
#define FILE_STATE_READ     0x08

#define IS_READABLE(Private)    (Private->FileState & FILE_STATE_READ)
#define IS_WRITEABLE(Private)   (Private->FileState & FILE_STATE_WRITE)

typedef struct GifFilePrivateType {
    GifWord FileState, FileHandle,  /* Where all this data goes to! */
        BitsPerPixel,     /* Bits per pixel (Codes uses at least this + 1). */
        ClearCode,   /* The CLEAR LZ code. */
        EOFCode,     /* The EOF LZ code. */
        RunningCode, /* The next code algorithm can generate. */
        RunningBits, /* The number of bits required to represent RunningCode. */
        MaxCode1,    /* 1 bigger than max. possible code, in RunningBits bits. */
        LastCode,    /* The code before the current code. */
        CrntCode,    /* Current algorithm code. */
        StackPtr,    /* For character stack (see below). */
        CrntShiftState;    /* Number of bits in CrntShiftDWord. */
    unsigned long CrntShiftDWord;   /* For bytes decomposition into codes. */
    unsigned long PixelCount;   /* Number of pixels in image. */
    FILE *File;    /* File as stream. */
    InputFunc Read;     /* function to read gif input (TVT) */
    OutputFunc Write;   /* function to write gif output (MRB) */
    GifByteType Buf[256];   /* Compressed input is buffered here. */
    GifByteType Stack[LZ_MAX_CODE]; /* Decoded pixels are stacked here. */
    GifByteType Suffix[LZ_MAX_CODE + 1];    /* So we can trace the codes. */
    GifPrefixType Prefix[LZ_MAX_CODE + 1];
    GifHashTableType *HashTable;
} GifFilePrivateType;

#define GifQprintf printf

static int _GifError = 0;

/*****************************************************************************
 * Print the last GIF error to stderr.
 ****************************************************************************/
#define PrintGifError() PrintGifError1(__LINE__)

static void
PrintGifError1(int line) {
    char *Err;

    switch (_GifError) {
    case E_GIF_ERR_OPEN_FAILED:
        Err = "Failed to open given file";
        break;
    case E_GIF_ERR_WRITE_FAILED:
        Err = "Failed to Write to given file";
        break;
    case E_GIF_ERR_HAS_SCRN_DSCR:
        Err = "Screen Descriptor already been set";
        break;
    case E_GIF_ERR_HAS_IMAG_DSCR:
        Err = "Image Descriptor is still active";
        break;
    case E_GIF_ERR_NO_COLOR_MAP:
        Err = "Neither Global Nor Local color map";
        break;
    case E_GIF_ERR_DATA_TOO_BIG:
        Err = "#Pixels bigger than Width * Height";
        break;
    case E_GIF_ERR_NOT_ENOUGH_MEM:
        Err = "Fail to allocate required memory";
        break;
    case E_GIF_ERR_DISK_IS_FULL:
        Err = "Write failed (disk full?)";
        break;
    case E_GIF_ERR_CLOSE_FAILED:
        Err = "Failed to close given file";
        break;
    case E_GIF_ERR_NOT_WRITEABLE:
        Err = "Given file was not opened for write";
        break;
    case D_GIF_ERR_OPEN_FAILED:
        Err = "Failed to open given file";
        break;
    case D_GIF_ERR_READ_FAILED:
        Err = "Failed to Read from given file";
        break;
    case D_GIF_ERR_NOT_GIF_FILE:
        Err = "Given file is NOT GIF file";
        break;
    case D_GIF_ERR_NO_SCRN_DSCR:
        Err = "No Screen Descriptor detected";
        break;
    case D_GIF_ERR_NO_IMAG_DSCR:
        Err = "No Image Descriptor detected";
        break;
    case D_GIF_ERR_NO_COLOR_MAP:
        Err = "Neither Global Nor Local color map";
        break;
    case D_GIF_ERR_WRONG_RECORD:
        Err = "Wrong record type detected";
        break;
    case D_GIF_ERR_DATA_TOO_BIG:
        Err = "#Pixels bigger than Width * Height";
        break;
    case D_GIF_ERR_NOT_ENOUGH_MEM:
        Err = "Fail to allocate required memory";
        break;
    case D_GIF_ERR_CLOSE_FAILED:
        Err = "Failed to close given file";
        break;
    case D_GIF_ERR_NOT_READABLE:
        Err = "Given file was not opened for read";
        break;
    case D_GIF_ERR_IMAGE_DEFECT:
        Err = "Image is defective, decoding aborted";
        break;
    case D_GIF_ERR_EOF_TOO_SOON:
        Err = "Image EOF detected, before image complete";
        break;
    default:
        Err = NULL;
        break;
    }
    if (Err != NULL)
        mylog("\n[%d] GIF-LIB error: %s.\n", line, Err);
    else
        mylog("\n[%d] GIF-LIB undefined error %d.\n", line, _GifError);
}


/******************************************************************************
 * Miscellaneous utility functions
 *****************************************************************************/

/* return smallest bitfield size n will fit in */
static int
BitSize(int n) {

    register int i;

    for (i = 1; i <= 8; i++)
        if ((1 << i) >= n)
            break;
    return (i);
}

/******************************************************************************
 * Color map object functions
 *****************************************************************************/

/*
 * Allocate a color map of given size; initialize with contents of
 * ColorMap if that pointer is non-NULL.
 */
static ColorMapObject *
MakeMapObject(int ColorCount,
              const GifColorType * ColorMap) {

    ColorMapObject *Object;

    /*** FIXME: Our ColorCount has to be a power of two.  Is it necessary to
     * make the user know that or should we automatically round up instead? */
    if (ColorCount != (1 << BitSize(ColorCount))) {
        return ((ColorMapObject *) NULL);
    }

    Object = (ColorMapObject *)malloc(sizeof(ColorMapObject));
    if (Object == (ColorMapObject *) NULL) {
        return ((ColorMapObject *) NULL);
    }

    Object->Colors = (GifColorType *)calloc(ColorCount, sizeof(GifColorType));
    if (Object->Colors == (GifColorType *) NULL) {
	free(Object);
        return ((ColorMapObject *) NULL);
    }

    Object->ColorCount = ColorCount;
    Object->BitsPerPixel = BitSize(ColorCount);

    if (ColorMap) {
        memcpy((char *)Object->Colors,
               (char *)ColorMap, ColorCount * sizeof(GifColorType));
    }

    return (Object);
}

/*
 * Free a color map object
 */
static
void
FreeMapObject(ColorMapObject * Object) {

    if (Object != NULL) {
        free(Object->Colors);
        free(Object);
        /*** FIXME:
         * When we are willing to break API we need to make this function
         * FreeMapObject(ColorMapObject **Object)
         * and do this assignment to NULL here:
         * *Object = NULL;
         */
    }
}

#ifdef DEBUG
static
void
DumpColorMap(ColorMapObject * Object,
             FILE * fp) {

    if (Object) {
        int i, j, Len = Object->ColorCount;

        for (i = 0; i < Len; i += 4) {
            for (j = 0; j < 4 && j < Len; j++) {
                fprintf(fp, "%3d: %02x %02x %02x   ", i + j,
                        Object->Colors[i + j].Red,
                        Object->Colors[i + j].Green,
                        Object->Colors[i + j].Blue);
            }
            fprintf(fp, "\n");
        }
    }
}
#endif /* DEBUG */

static
void
FreeExtension(SavedImage * Image)
{
    ExtensionBlock *ep;

    if ((Image == NULL) || (Image->ExtensionBlocks == NULL)) {
        return;
    }
    for (ep = Image->ExtensionBlocks;
         ep < (Image->ExtensionBlocks + Image->ExtensionBlockCount); ep++)
        (void)free((char *)ep->Bytes);
    free((char *)Image->ExtensionBlocks);
    Image->ExtensionBlocks = NULL;
}

/******************************************************************************
 * Image block allocation functions
 ******************************************************************************/

static
void
FreeSavedImages(GifFileType * GifFile) {

    SavedImage *sp;

    if ((GifFile == NULL) || (GifFile->SavedImages == NULL)) {
        return;
    }
    for (sp = GifFile->SavedImages;
         sp < GifFile->SavedImages + GifFile->ImageCount; sp++) {
        if (sp->ImageDesc.ColorMap) {
            FreeMapObject(sp->ImageDesc.ColorMap);
            sp->ImageDesc.ColorMap = NULL;
        }

        if (sp->RasterBits)
            free((char *)sp->RasterBits);

        if (sp->ExtensionBlocks)
            FreeExtension(sp);
    }
    free((char *)GifFile->SavedImages);
    GifFile->SavedImages=NULL;
}

#define COMMENT_EXT_FUNC_CODE 0xfe  /* Extension function code for
                                       comment. */

/* avoid extra function call in case we use fread (TVT) */
#define READ(_gif,_buf,_len)                                            \
    (((GifFilePrivateType*)_gif->Private)->Read ?                       \
     ((GifFilePrivateType*)_gif->Private)->Read(_gif,_buf,_len) :       \
     fread(_buf,1,_len,((GifFilePrivateType*)_gif->Private)->File))

static int DGifGetWord(GifFileType *GifFile, GifWord *Word);
static int DGifSetupDecompress(GifFileType *GifFile);
static int DGifDecompressLine(GifFileType *GifFile, GifPixelType *Line,
                              int LineLen);
static int DGifGetPrefixChar(GifPrefixType *Prefix, int Code, int ClearCode);
static int DGifDecompressInput(GifFileType *GifFile, int *Code);
static int DGifBufferedInput(GifFileType *GifFile, GifByteType *Buf,
                             GifByteType *NextByte);

/******************************************************************************
 * Open a new gif file for read, given by its name.
 * Returns GifFileType pointer dynamically allocated which serves as the gif
 * info record. _GifError is cleared if succesfull.
 *****************************************************************************/
static
GifFileType *
DGifOpenFileName(const char *FileName) {
    int FileHandle;
    GifFileType *GifFile;

    if ((FileHandle = open(FileName, O_RDONLY
#ifdef WIN32
                           | O_BINARY
#endif
             )) == -1) {
        _GifError = D_GIF_ERR_OPEN_FAILED;
        return NULL;
    }

    GifFile = DGifOpenFileHandle(FileHandle);
    return GifFile;
}

/******************************************************************************
 * Update a new gif file, given its file handle.
 * Returns GifFileType pointer dynamically allocated which serves as the gif
 * info record. _GifError is cleared if succesfull.
 *****************************************************************************/
static
GifFileType *
DGifOpenFileHandle(int FileHandle) {

    unsigned char Buf[GIF_STAMP_LEN + 1];
    GifFileType *GifFile;
    GifFilePrivateType *Private;
    FILE *f;

    GifFile = (GifFileType *)malloc(sizeof(GifFileType));
    if (GifFile == NULL) {
        _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
        close(FileHandle);
        return NULL;
    }

    memset(GifFile, '\0', sizeof(GifFileType));

    Private = (GifFilePrivateType *)malloc(sizeof(GifFilePrivateType));
    if (Private == NULL) {
        _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
        close(FileHandle);
        free((char *)GifFile);
        return NULL;
    }

#ifdef WIN32
    setmode(FileHandle, O_BINARY);
#endif
    f = fdopen(FileHandle, "rb");    /* Make it into a stream: */
#ifdef WIN32
    setvbuf(f, NULL, _IOFBF, GIF_FILE_BUFFER_SIZE);
#endif

    GifFile->Private = (VoidPtr)Private;
    Private->FileHandle = FileHandle;
    Private->File = f;
    Private->FileState = FILE_STATE_READ;
    Private->Read = 0;    /* don't use alternate input method (TVT) */
    GifFile->UserData = 0;    /* TVT */

    /* Lets see if this is a GIF file: */
    if (READ(GifFile, Buf, GIF_STAMP_LEN) != GIF_STAMP_LEN) {
        T;
        _GifError = D_GIF_ERR_READ_FAILED;
        fclose(f);
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    /* The GIF Version number is ignored at this time. Maybe we should do
     * something more useful with it.  */
    Buf[GIF_STAMP_LEN] = 0;
    if (strncmp(GIF_STAMP, (char *)Buf, GIF_VERSION_POS) != 0) {
        _GifError = D_GIF_ERR_NOT_GIF_FILE;
        fclose(f);
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    if (DGifGetScreenDesc(GifFile) == GIF_ERROR) {
        fclose(f);
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    _GifError = 0;

    return GifFile;
}

/******************************************************************************
 * This routine should be called before any other DGif calls. Note that
 * this routine is called automatically from DGif file open routines.
 *****************************************************************************/
static
int
DGifGetScreenDesc(GifFileType * GifFile) {

    int i, BitsPerPixel;
    GifByteType Buf[3];
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    /* Put the screen descriptor into the file: */
    if (DGifGetWord(GifFile, &GifFile->SWidth) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->SHeight) == GIF_ERROR)
        return GIF_ERROR;

    if (READ(GifFile, Buf, 3) != 3) {
        T;
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }
    GifFile->SColorResolution = (((Buf[0] & 0x70) + 1) >> 4) + 1;
    BitsPerPixel = (Buf[0] & 0x07) + 1;
    GifFile->SBackGroundColor = Buf[1];
    if (Buf[0] & 0x80) {    /* Do we have global color map? */

        GifFile->SColorMap = MakeMapObject(1 << BitsPerPixel, NULL);
        if (GifFile->SColorMap == NULL) {
            _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }

        /* Get the global color map: */
        for (i = 0; i < GifFile->SColorMap->ColorCount; i++) {
            if (READ(GifFile, Buf, 3) != 3) {
                T;
                FreeMapObject(GifFile->SColorMap);
                GifFile->SColorMap = NULL;
                _GifError = D_GIF_ERR_READ_FAILED;
                return GIF_ERROR;
            }
            GifFile->SColorMap->Colors[i].Red = Buf[0];
            GifFile->SColorMap->Colors[i].Green = Buf[1];
            GifFile->SColorMap->Colors[i].Blue = Buf[2];
        }
    } else {
        GifFile->SColorMap = NULL;
    }

    return GIF_OK;
}

/******************************************************************************
 * This routine should be called before any attempt to read an image.
 *****************************************************************************/
static
int
DGifGetRecordType(GifFileType * GifFile,
                  GifRecordType * Type) {

    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    if (READ(GifFile, &Buf, 1) != 1) {
        T;
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }

    switch (Buf) {
    case ',':
        *Type = IMAGE_DESC_RECORD_TYPE;
        break;
    case '!':
        *Type = EXTENSION_RECORD_TYPE;
        break;
    case ';':
        *Type = TERMINATE_RECORD_TYPE;
        break;
    default:
        *Type = UNDEFINED_RECORD_TYPE;
        _GifError = D_GIF_ERR_WRONG_RECORD;
        return GIF_ERROR;
    }

    return GIF_OK;
}

/******************************************************************************
 * This routine should be called before any attempt to read an image.
 * Note it is assumed the Image desc. header (',') has been read.
 *****************************************************************************/
static
int
DGifGetImageDesc(GifFileType * GifFile) {

    int i, BitsPerPixel;
    GifByteType Buf[3];
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;
    SavedImage *sp;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    if (DGifGetWord(GifFile, &GifFile->Image.Left) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->Image.Top) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->Image.Width) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->Image.Height) == GIF_ERROR)
        return GIF_ERROR;
    if (READ(GifFile, Buf, 1) != 1) {
        T;
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }
    BitsPerPixel = (Buf[0] & 0x07) + 1;
    GifFile->Image.Interlace = (Buf[0] & 0x40);
    if (Buf[0] & 0x80) {    /* Does this image have local color map? */

        /*** FIXME: Why do we check both of these in order to do this?
         * Why do we have both Image and SavedImages? */
        if (GifFile->Image.ColorMap && GifFile->SavedImages == NULL)
            FreeMapObject(GifFile->Image.ColorMap);

        GifFile->Image.ColorMap = MakeMapObject(1 << BitsPerPixel, NULL);
        if (GifFile->Image.ColorMap == NULL) {
            _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }

        /* Get the image local color map: */
        for (i = 0; i < GifFile->Image.ColorMap->ColorCount; i++) {
            if (READ(GifFile, Buf, 3) != 3) {
                T;
                FreeMapObject(GifFile->Image.ColorMap);
                _GifError = D_GIF_ERR_READ_FAILED;
                GifFile->Image.ColorMap = NULL;
                return GIF_ERROR;
            }
            GifFile->Image.ColorMap->Colors[i].Red = Buf[0];
            GifFile->Image.ColorMap->Colors[i].Green = Buf[1];
            GifFile->Image.ColorMap->Colors[i].Blue = Buf[2];
        }
    } else if (GifFile->Image.ColorMap) {
        FreeMapObject(GifFile->Image.ColorMap);
        GifFile->Image.ColorMap = NULL;
    }

    if (GifFile->SavedImages) {
        if ((GifFile->SavedImages = (SavedImage *)realloc(GifFile->SavedImages,
                                                          sizeof(SavedImage) *
                                                          (GifFile->ImageCount + 1))) == NULL) {
            _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }
    } else {
        if ((GifFile->SavedImages =
             (SavedImage *) malloc(sizeof(SavedImage))) == NULL) {
            _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }
    }

    sp = &GifFile->SavedImages[GifFile->ImageCount];
    memcpy(&sp->ImageDesc, &GifFile->Image, sizeof(GifImageDesc));
    if (GifFile->Image.ColorMap != NULL) {
        sp->ImageDesc.ColorMap = MakeMapObject(
            GifFile->Image.ColorMap->ColorCount,
            GifFile->Image.ColorMap->Colors);
        if (sp->ImageDesc.ColorMap == NULL) {
            _GifError = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }
    }
    sp->RasterBits = (unsigned char *)NULL;
    sp->ExtensionBlockCount = 0;
    sp->ExtensionBlocks = (ExtensionBlock *) NULL;

    GifFile->ImageCount++;

    Private->PixelCount = (long)GifFile->Image.Width *
        (long)GifFile->Image.Height;

    DGifSetupDecompress(GifFile);  /* Reset decompress algorithm parameters. */

    return GIF_OK;
}

/******************************************************************************
 * Get one full scanned line (Line) of length LineLen from GIF file.
 *****************************************************************************/
static
int
DGifGetLine(GifFileType * GifFile,
            GifPixelType * Line,
            int LineLen) {

    GifByteType *Dummy;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    if (!LineLen)
        LineLen = GifFile->Image.Width;

    if ((Private->PixelCount -= LineLen) > 0xffff0000UL) {
        _GifError = D_GIF_ERR_DATA_TOO_BIG;
        return GIF_ERROR;
    }

    if (DGifDecompressLine(GifFile, Line, LineLen) == GIF_OK) {
        if (Private->PixelCount == 0) {
            /* We probably would not be called any more, so lets clean
             * everything before we return: need to flush out all rest of
             * image until empty block (size 0) detected. We use GetCodeNext. */
            do
                if (DGifGetCodeNext(GifFile, &Dummy) == GIF_ERROR)
                    return GIF_ERROR;
            while (Dummy != NULL) ;
        }
        return GIF_OK;
    } else
        return GIF_ERROR;
}

/******************************************************************************
 * Get an extension block (see GIF manual) from gif file. This routine only
 * returns the first data block, and DGifGetExtensionNext should be called
 * after this one until NULL extension is returned.
 * The Extension should NOT be freed by the user (not dynamically allocated).
 * Note it is assumed the Extension desc. header ('!') has been read.
 *****************************************************************************/
static
int
DGifGetExtension(GifFileType * GifFile,
                 int *ExtCode,
                 GifByteType ** Extension) {

    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    if (READ(GifFile, &Buf, 1) != 1) {
        T;
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }
    *ExtCode = Buf;

    return DGifGetExtensionNext(GifFile, Extension);
}

/******************************************************************************
 * Get a following extension block (see GIF manual) from gif file. This
 * routine should be called until NULL Extension is returned.
 * The Extension should NOT be freed by the user (not dynamically allocated).
 *****************************************************************************/
static
int
DGifGetExtensionNext(GifFileType * GifFile,
                     GifByteType ** Extension) {

    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (READ(GifFile, &Buf, 1) != 1) {
        T;
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }
    if (Buf > 0) {
        *Extension = Private->Buf;    /* Use private unused buffer. */
        (*Extension)[0] = Buf;  /* Pascal strings notation (pos. 0 is len.). */
        if (READ(GifFile, &((*Extension)[1]), Buf) != Buf) {
            T;
            _GifError = D_GIF_ERR_READ_FAILED;
            return GIF_ERROR;
        }
    } else
        *Extension = NULL;

    return GIF_OK;
}

/******************************************************************************
 * This routine should be called last, to close the GIF file.
 *****************************************************************************/
static
int
DGifCloseFile(GifFileType * GifFile) {

    GifFilePrivateType *Private;
    FILE *File;

    if (GifFile == NULL)
        return GIF_ERROR;

    Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        _GifError = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    File = Private->File;

    if (GifFile->Image.ColorMap) {
        FreeMapObject(GifFile->Image.ColorMap);
        GifFile->Image.ColorMap = NULL;
    }

    if (GifFile->SColorMap) {
        FreeMapObject(GifFile->SColorMap);
        GifFile->SColorMap = NULL;
    }

    if (Private) {
        free((char *)Private);
        Private = NULL;
    }

    if (GifFile->SavedImages) {
        FreeSavedImages(GifFile);
        GifFile->SavedImages = NULL;
    }

    free(GifFile);

    if (File && (fclose(File) != 0)) {
        _GifError = D_GIF_ERR_CLOSE_FAILED;
        return GIF_ERROR;
    }
    return GIF_OK;
}

/******************************************************************************
 * Get 2 bytes (word) from the given file:
 *****************************************************************************/
static int
DGifGetWord(GifFileType * GifFile,
            GifWord *Word) {

    unsigned char c[2];

    if (READ(GifFile, c, 2) != 2) {
        T;
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }

    *Word = (((unsigned int)c[1]) << 8) + c[0];
    return GIF_OK;
}

/******************************************************************************
 * Continue to get the image code in compressed form. This routine should be
 * called until NULL block is returned.
 * The block should NOT be freed by the user (not dynamically allocated).
 *****************************************************************************/
static
int
DGifGetCodeNext(GifFileType * GifFile,
                GifByteType ** CodeBlock) {

    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (READ(GifFile, &Buf, 1) != 1) {
        T;
        _GifError = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }

    if (Buf > 0) {
        *CodeBlock = Private->Buf;    /* Use private unused buffer. */
        (*CodeBlock)[0] = Buf;  /* Pascal strings notation (pos. 0 is len.). */
        if (READ(GifFile, &((*CodeBlock)[1]), Buf) != Buf) {
            T;
            _GifError = D_GIF_ERR_READ_FAILED;
            return GIF_ERROR;
        }
    } else {
        *CodeBlock = NULL;
        Private->Buf[0] = 0;    /* Make sure the buffer is empty! */
        Private->PixelCount = 0;    /* And local info. indicate image read. */
    }

    return GIF_OK;
}

/******************************************************************************
 * Setup the LZ decompression for this image:
 *****************************************************************************/
static int
DGifSetupDecompress(GifFileType * GifFile) {

    int i, BitsPerPixel;
    GifByteType CodeSize;
    GifPrefixType *Prefix;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    READ(GifFile, &CodeSize, 1);    /* Read Code size from file. */
    BitsPerPixel = CodeSize;

    Private->Buf[0] = 0;    /* Input Buffer empty. */
    Private->BitsPerPixel = BitsPerPixel;
    Private->ClearCode = (1 << BitsPerPixel);
    Private->EOFCode = Private->ClearCode + 1;
    Private->RunningCode = Private->EOFCode + 1;
    Private->RunningBits = BitsPerPixel + 1;    /* Number of bits per code. */
    Private->MaxCode1 = 1 << Private->RunningBits;    /* Max. code + 1. */
    Private->StackPtr = 0;    /* No pixels on the pixel stack. */
    Private->LastCode = NO_SUCH_CODE;
    Private->CrntShiftState = 0;    /* No information in CrntShiftDWord. */
    Private->CrntShiftDWord = 0;

    Prefix = Private->Prefix;
    for (i = 0; i <= LZ_MAX_CODE; i++)
        Prefix[i] = NO_SUCH_CODE;

    return GIF_OK;
}

/******************************************************************************
 * The LZ decompression routine:
 * This version decompress the given gif file into Line of length LineLen.
 * This routine can be called few times (one per scan line, for example), in
 * order the complete the whole image.
 *****************************************************************************/
static int
DGifDecompressLine(GifFileType * GifFile,
                   GifPixelType * Line,
                   int LineLen) {

    int i = 0;
    int j, CrntCode, EOFCode, ClearCode, CrntPrefix, LastCode, StackPtr;
    GifByteType *Stack, *Suffix;
    GifPrefixType *Prefix;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    StackPtr = Private->StackPtr;
    Prefix = Private->Prefix;
    Suffix = Private->Suffix;
    Stack = Private->Stack;
    EOFCode = Private->EOFCode;
    ClearCode = Private->ClearCode;
    LastCode = Private->LastCode;

    if (StackPtr > LZ_MAX_CODE) {
        return GIF_ERROR;
    }

    if (StackPtr != 0) {
        /* Let pop the stack off before continueing to read the gif file: */
        while (StackPtr != 0 && i < LineLen)
            Line[i++] = Stack[--StackPtr];
    }

    while (i < LineLen) {    /* Decode LineLen items. */
        if (DGifDecompressInput(GifFile, &CrntCode) == GIF_ERROR)
            return GIF_ERROR;

        if (CrntCode == EOFCode) {
            /* Note however that usually we will not be here as we will stop
             * decoding as soon as we got all the pixel, or EOF code will
             * not be read at all, and DGifGetLine/Pixel clean everything.  */
            if (i != LineLen - 1 || Private->PixelCount != 0) {
                _GifError = D_GIF_ERR_EOF_TOO_SOON;
                return GIF_ERROR;
            }
            i++;
        } else if (CrntCode == ClearCode) {
            /* We need to start over again: */
            for (j = 0; j <= LZ_MAX_CODE; j++)
                Prefix[j] = NO_SUCH_CODE;
            Private->RunningCode = Private->EOFCode + 1;
            Private->RunningBits = Private->BitsPerPixel + 1;
            Private->MaxCode1 = 1 << Private->RunningBits;
            LastCode = Private->LastCode = NO_SUCH_CODE;
        } else {
            /* Its regular code - if in pixel range simply add it to output
             * stream, otherwise trace to codes linked list until the prefix
             * is in pixel range: */
            if (CrntCode < ClearCode) {
                /* This is simple - its pixel scalar, so add it to output: */
                Line[i++] = CrntCode;
            } else {
                /* Its a code to needed to be traced: trace the linked list
                 * until the prefix is a pixel, while pushing the suffix
                 * pixels on our stack. If we done, pop the stack in reverse
                 * (thats what stack is good for!) order to output.  */
                if (Prefix[CrntCode] == NO_SUCH_CODE) {
                    /* Only allowed if CrntCode is exactly the running code:
                     * In that case CrntCode = XXXCode, CrntCode or the
                     * prefix code is last code and the suffix char is
                     * exactly the prefix of last code! */
                    if (CrntCode == Private->RunningCode - 2) {
                        CrntPrefix = LastCode;
                        Suffix[Private->RunningCode - 2] =
                            Stack[StackPtr++] = DGifGetPrefixChar(Prefix,
                                                                  LastCode,
                                                                  ClearCode);
                    } else {
                        _GifError = D_GIF_ERR_IMAGE_DEFECT;
                        return GIF_ERROR;
                    }
                } else
                    CrntPrefix = CrntCode;

                /* Now (if image is O.K.) we should not get an NO_SUCH_CODE
                 * During the trace. As we might loop forever, in case of
                 * defective image, we count the number of loops we trace
                 * and stop if we got LZ_MAX_CODE. obviously we can not
                 * loop more than that.  */
                j = 0;
                while (j++ <= LZ_MAX_CODE &&
                       CrntPrefix > ClearCode && CrntPrefix <= LZ_MAX_CODE) {
                    Stack[StackPtr++] = Suffix[CrntPrefix];
                    CrntPrefix = Prefix[CrntPrefix];
                }
                if (j >= LZ_MAX_CODE || CrntPrefix > LZ_MAX_CODE) {
                    _GifError = D_GIF_ERR_IMAGE_DEFECT;
                    return GIF_ERROR;
                }
                /* Push the last character on stack: */
                Stack[StackPtr++] = CrntPrefix;

                /* Now lets pop all the stack into output: */
                while (StackPtr != 0 && i < LineLen)
                    Line[i++] = Stack[--StackPtr];
            }
            if (LastCode != NO_SUCH_CODE) {
                Prefix[Private->RunningCode - 2] = LastCode;

                if (CrntCode == Private->RunningCode - 2) {
                    /* Only allowed if CrntCode is exactly the running code:
                     * In that case CrntCode = XXXCode, CrntCode or the
                     * prefix code is last code and the suffix char is
                     * exactly the prefix of last code! */
                    Suffix[Private->RunningCode - 2] =
                        DGifGetPrefixChar(Prefix, LastCode, ClearCode);
                } else {
                    Suffix[Private->RunningCode - 2] =
                        DGifGetPrefixChar(Prefix, CrntCode, ClearCode);
                }
            }
            LastCode = CrntCode;
        }
    }

    Private->LastCode = LastCode;
    Private->StackPtr = StackPtr;

    return GIF_OK;
}

/******************************************************************************
 * Routine to trace the Prefixes linked list until we get a prefix which is
 * not code, but a pixel value (less than ClearCode). Returns that pixel value.
 * If image is defective, we might loop here forever, so we limit the loops to
 * the maximum possible if image O.k. - LZ_MAX_CODE times.
 *****************************************************************************/
static int
DGifGetPrefixChar(GifPrefixType *Prefix,
                  int Code,
                  int ClearCode) {

    int i = 0;

    while (Code > ClearCode && i++ <= LZ_MAX_CODE) {
        if (Code > LZ_MAX_CODE) {
            return NO_SUCH_CODE;
        }
        Code = Prefix[Code];
    }
    return Code;
}

/******************************************************************************
 * The LZ decompression input routine:
 * This routine is responsable for the decompression of the bit stream from
 * 8 bits (bytes) packets, into the real codes.
 * Returns GIF_OK if read succesfully.
 *****************************************************************************/
static int
DGifDecompressInput(GifFileType * GifFile,
                    int *Code) {

    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    GifByteType NextByte;
    static unsigned short CodeMasks[] = {
        0x0000, 0x0001, 0x0003, 0x0007,
        0x000f, 0x001f, 0x003f, 0x007f,
        0x00ff, 0x01ff, 0x03ff, 0x07ff,
        0x0fff
    };
    /* The image can't contain more than LZ_BITS per code. */
    if (Private->RunningBits > LZ_BITS) {
        _GifError = D_GIF_ERR_IMAGE_DEFECT;
        return GIF_ERROR;
    }

    while (Private->CrntShiftState < Private->RunningBits) {
        /* Needs to get more bytes from input stream for next code: */
        if (DGifBufferedInput(GifFile, Private->Buf, &NextByte) == GIF_ERROR) {
            return GIF_ERROR;
        }
        Private->CrntShiftDWord |=
            ((unsigned long)NextByte) << Private->CrntShiftState;
        Private->CrntShiftState += 8;
    }
    *Code = Private->CrntShiftDWord & CodeMasks[Private->RunningBits];

    Private->CrntShiftDWord >>= Private->RunningBits;
    Private->CrntShiftState -= Private->RunningBits;

    /* If code cannot fit into RunningBits bits, must raise its size. Note
     * however that codes above 4095 are used for special signaling.
     * If we're using LZ_BITS bits already and we're at the max code, just
     * keep using the table as it is, don't increment Private->RunningCode.
     */
    if (Private->RunningCode < LZ_MAX_CODE + 2 &&
        ++Private->RunningCode > Private->MaxCode1 &&
        Private->RunningBits < LZ_BITS) {
        Private->MaxCode1 <<= 1;
        Private->RunningBits++;
    }
    return GIF_OK;
}

/******************************************************************************
 * This routines read one gif data block at a time and buffers it internally
 * so that the decompression routine could access it.
 * The routine returns the next byte from its internal buffer (or read next
 * block in if buffer empty) and returns GIF_OK if succesful.
 *****************************************************************************/
static int
DGifBufferedInput(GifFileType * GifFile,
                  GifByteType * Buf,
                  GifByteType * NextByte) {

    if (Buf[0] == 0) {
        /* Needs to read the next buffer - this one is empty: */
        if (READ(GifFile, Buf, 1) != 1) {
            T;
            _GifError = D_GIF_ERR_READ_FAILED;
            return GIF_ERROR;
        }
        /* There shouldn't be any empty data blocks here as the LZW spec
         * says the LZW termination code should come first.  Therefore we
         * shouldn't be inside this routine at that point.
         */
        if (Buf[0] == 0) {
            _GifError = D_GIF_ERR_IMAGE_DEFECT;
            return GIF_ERROR;
        }

        if (READ(GifFile, &Buf[1], Buf[0]) != Buf[0]) {
            T;
            _GifError = D_GIF_ERR_READ_FAILED;
            return GIF_ERROR;
        }
        *NextByte = Buf[1];
        Buf[1] = 2;    /* We use now the second place as last char read! */
        Buf[0]--;
    } else {
        *NextByte = Buf[Buf[1]++];
        Buf[0]--;
    }

    return GIF_OK;
}


/* Make some variables global, so we could access them faster: */
static int ImageNum = 0,
    BackGround = 0,
    InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */
    InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */
static ColorMapObject *ColorMap;

#define NUM_ADS 10
struct frame {
    struct frame *next;
    cairo_surface_t *surface;
    gint x, y, width, height, stride;
};
struct ad {
    gint swidth, sheight;
    struct frame frames;
};
static struct ad advertisements[NUM_ADS];
static gint num_ads = 0, current_ad = 0, current_frame = 0;

void display_ad_window(void);

static void save_advertisement(GifRowType *ScreenBuffer,
			       gint swidth, gint sheight, gint x, gint y,
			       gint width, gint height);

static gint read_gif(gchar *FileName)
{
    int	i, j, Size, Row, Col, Width, Height, ExtCode/*, Count*/;
    GifRecordType RecordType;
    GifByteType *Extension;
    GifRowType *ScreenBuffer;
    GifFileType *GifFile;

    //mylog("FILE %s\n", FileName);

    if (num_ads >= NUM_ADS)
        return 0;

    if ((GifFile = DGifOpenFileName(FileName)) == NULL) {
        PrintGifError();
        return(EXIT_FAILURE);
    }

    if ((ScreenBuffer = (GifRowType *)
         malloc(GifFile->SHeight * sizeof(GifRowType *))) == NULL)
        GIF_EXIT("Failed to allocate memory required, aborted.");

    Size = GifFile->SWidth * sizeof(GifPixelType);/* Size in bytes one row.*/
    if ((ScreenBuffer[0] = (GifRowType) malloc(Size)) == NULL) /* First row. */
        GIF_EXIT("Failed to allocate memory required, aborted.");

    for (i = 0; i < GifFile->SWidth; i++)  /* Set its color to BackGround. */
        ScreenBuffer[0][i] = GifFile->SBackGroundColor;
    for (i = 1; i < GifFile->SHeight; i++) {
        /* Allocate the other rows, and set their color to background too: */
        if ((ScreenBuffer[i] = (GifRowType) malloc(Size)) == NULL)
            GIF_EXIT("Failed to allocate memory required, aborted.");

        memcpy(ScreenBuffer[i], ScreenBuffer[0], Size);
    }

    /* Scan the content of the GIF file and load the image(s) in: */
    do {
        if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR) {
            PrintGifError();
            return(EXIT_FAILURE);
        }
        switch (RecordType) {
        case IMAGE_DESC_RECORD_TYPE:
            if (DGifGetImageDesc(GifFile) == GIF_ERROR) {
                PrintGifError();
                return(EXIT_FAILURE);
            }
            Row = GifFile->Image.Top; /* Image Position relative to Screen. */
            Col = GifFile->Image.Left;
            Width = GifFile->Image.Width;
            Height = GifFile->Image.Height;

#if 0
            GifQprintf("\nImage %d at (%d, %d) [%dx%d]:     ",
                       ++ImageNum, Col, Row, Width, Height);
            mylog("\n\nleft=%d width=%d swidth=%d\ntop=%d height=%d sheight=%d\n\n",
                   GifFile->Image.Left, GifFile->Image.Width, GifFile->SWidth,
                   GifFile->Image.Top, GifFile->Image.Height, GifFile->SHeight);
#endif

            if (GifFile->Image.Left + GifFile->Image.Width > GifFile->SWidth ||
                GifFile->Image.Top + GifFile->Image.Height > GifFile->SHeight) {
                fprintf(stderr, "Image %d is not confined to screen dimension, aborted.\n",ImageNum);
                return(EXIT_FAILURE);
            }
            if (GifFile->Image.Interlace) {
                /* Need to perform 4 passes on the images: */
                for (/*Count =*/ i = 0; i < 4; i++)
                    for (j = Row + InterlacedOffset[i]; j < Row + Height;
                         j += InterlacedJumps[i]) {
                        //GifQmylog("\b\b\b\b%-4d", Count++);
                        if (DGifGetLine(GifFile, &ScreenBuffer[j][Col],
                                        Width) == GIF_ERROR) {
                            PrintGifError();
                            return(EXIT_FAILURE);
                        }
                    }
            }
            else {
                for (i = 0; i < Height; i++) {
                    //GifQmylog("\b\b\b\b%-4d", i);
                    if (DGifGetLine(GifFile, &ScreenBuffer[Row++][Col],
                                    Width) == GIF_ERROR) {
                        PrintGifError();
                        return(EXIT_FAILURE);
                    }
                }
            }

            /* Lets dump it - set the global variables required and do it: */
            BackGround = GifFile->SBackGroundColor;
            ColorMap = (GifFile->Image.ColorMap
                        ? GifFile->Image.ColorMap
                        : GifFile->SColorMap);
            if (ColorMap == NULL) {
                fprintf(stderr, "Gif Image does not have a colormap\n");
                return(EXIT_FAILURE);
            }

            save_advertisement(ScreenBuffer, GifFile->SWidth, GifFile->SHeight,
                               GifFile->Image.Left, GifFile->Image.Top,
                               GifFile->Image.Width, GifFile->Image.Height);

            break;
        case EXTENSION_RECORD_TYPE:
            /* Skip any extension blocks in file: */
            if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR) {
                PrintGifError();
                return(EXIT_FAILURE);
            }
            while (Extension != NULL) {
                if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR) {
                    PrintGifError();
                    return(EXIT_FAILURE);
                }
            }
            break;
        case TERMINATE_RECORD_TYPE:
            break;
        default:		    /* Should be traps by DGifGetRecordType. */
            break;
        }
    } while (RecordType != TERMINATE_RECORD_TYPE);

#if 0
    /* Lets dump it - set the global variables required and do it: */
    BackGround = GifFile->SBackGroundColor;
    ColorMap = (GifFile->Image.ColorMap
                ? GifFile->Image.ColorMap
                : GifFile->SColorMap);
    if (ColorMap == NULL) {
        fprintf(stderr, "Gif Image does not have a colormap\n");
        return(EXIT_FAILURE);
    }

    display_window(ScreenBuffer, GifFile->SWidth, GifFile->SHeight);
#endif
    if (DGifCloseFile(GifFile) == GIF_ERROR) {
        PrintGifError();
        return(EXIT_FAILURE);
    }

    num_ads++;

    return 0;
}

/******************************************************************************
 * The real dumping routine.						      *
 ******************************************************************************/

gboolean show_competitor_names = FALSE;
static GtkWindow *ad_window = NULL;
static gboolean comp_names_pending = FALSE;
static gboolean no_ads = FALSE;
static time_t comp_names_start = 0;
static gchar category[32];
static gchar b_last[32], w_last[32];
static gchar b_first[32], w_first[32];
static gchar b_country[32], w_country[32];
static gchar b_club[32], w_club[32];
static GtkWidget *ok_button = NULL;
static gint cat_round = 0;

static struct frame *get_current_frame(GtkWidget *widget)
{
    gint i;
    struct frame *f = advertisements[current_ad].frames.next;

#if 0
    if (f && f->next == NULL && current_frame < 20)
        return f;
#endif
    for (i = 0; i < current_frame; i++) {
        if (!f)
            break;
        f = f->next;
    }

    return f;
}

static gboolean current_ad_has_one_frame(void)
{
    struct frame *f = advertisements[current_ad].frames.next;

    if (f && f->next == NULL)
        return TRUE;
    return FALSE;
}

static gint rmin, rtsec, rsec, rflags, judogi_control_flags;
static gboolean rrest, judogi_control;

void set_competitor_window_rest_time(gint min, gint tsec, gint sec, gboolean rest, gint flags)
{
    if (!ad_window)
        return;

    if (rrest == rest && rsec == sec)
        return;

    rmin = min;
    rtsec = tsec;
    rsec = sec;
    rrest = rest;
    rflags = flags;

    gtk_widget_queue_draw(GTK_WIDGET(ad_window));
    //gdk_window_invalidate_rect(GDK_WINDOW(ad_window), NULL, TRUE);
    /*if (gtk_widget_get_window(widget)) {
        gdk_window_invalidate_rect(gtk_widget_get_window(widget), NULL, TRUE);
        }*/
}

void set_competitor_window_judogi_control(gboolean control, gint flags)
{
    if (!ad_window)
        return;

    if (judogi_control == control && judogi_control_flags == flags) {
        if (judogi_control)
            gtk_widget_queue_draw(GTK_WIDGET(ad_window));
        //gdk_window_invalidate_rect(GDK_WINDOW(ad_window), NULL, TRUE);
        return;
    }

    judogi_control = control;
    judogi_control_flags = flags;

    gtk_widget_queue_draw(GTK_WIDGET(ad_window));
    //gdk_window_invalidate_rect(GDK_WINDOW(ad_window), NULL, TRUE);
}

static gdouble name_start = 10.0;

static void draw_flag(cairo_t *c, gdouble y, gdouble height, gchar *country, gdouble flag_size)
{
    gchar buf[32];
    snprintf(buf, sizeof(buf), "%s.png", country);
    gchar *file = g_build_filename(installation_dir, "etc", "flags-ioc", buf, NULL);
    cairo_surface_t *image = cairo_image_surface_create_from_png(file);
    if (image && cairo_surface_status(image) == CAIRO_STATUS_SUCCESS) {
        gdouble icon_w = cairo_image_surface_get_width(image);
        gdouble icon_h = cairo_image_surface_get_height(image);
        gdouble flag_h = height*flag_size;
        gdouble scale = flag_h/icon_h;
        gdouble y1 = (y+(height-flag_h)/2.0);
        cairo_save(c);
        cairo_scale(c, scale, scale);
        cairo_set_source_surface(c, image, 1.0/scale, y1/scale);
        cairo_paint(c);
        cairo_restore(c);
        gdouble flag_w = flag_h*icon_w/icon_h;
        if (flag_w > name_start) name_start = flag_w;

        // rectangle
        cairo_set_source_rgb(c, 0.0, 0.0, 0.0);
        cairo_set_line_width(c, 3);
        cairo_move_to(c, 1, y1);
        cairo_line_to(c, flag_w+1, y1);
        cairo_line_to(c, flag_w+1, y1+flag_h);
        cairo_line_to(c, 1, y1+flag_h);
        cairo_line_to(c, 1, y1);
        cairo_stroke(c);
    }

    cairo_surface_destroy(image);
    g_free(file);
}

static cairo_surface_t *white_ctl = NULL, *blue_ctl = NULL;

static gboolean expose_ad(GtkWidget *widget, GdkEventExpose *event, gpointer userdata)
{
    gint width, height;
    struct frame *frame = NULL;

    if (!white_ctl) {
        gchar *file = g_build_filename(installation_dir, "etc", "png", "white-ctl.png", NULL);
        white_ctl = cairo_image_surface_create_from_png(file);
        g_free(file);
    }
    if (!blue_ctl) {
        gchar *file = g_build_filename(installation_dir, "etc", "png", "blue-ctl.png", NULL);
        blue_ctl = cairo_image_surface_create_from_png(file);
        g_free(file);
    }
    
    if (!no_ads)
        frame = get_current_frame(widget);

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    if (!frame) {
        if (comp_names_pending == FALSE)
            return FALSE;

#define FIRST_BLOCK_C 0.25
#define FIRST_BLOCK_START  0.0
#define SECOND_BLOCK_START  (FIRST_BLOCK_C*height)
#define THIRD_BLOCK_START ((1.0+FIRST_BLOCK_C)*height/2.0)
#define FIRST_BLOCK_HEIGHT (FIRST_BLOCK_C*height)
#define OTHER_BLOCK_HEIGHT ((height-FIRST_BLOCK_HEIGHT)/2.0)

        cairo_text_extents_t extents;
        cairo_t *c = (cairo_t *)event;
        
        ad_svg_handle.c = c;
        ad_svg_handle.paper_width = width;
        ad_svg_handle.paper_height = height;
        if (paint_svg_common(&ad_svg_handle))
            return FALSE;

        if (ok_button)
            gtk_widget_show(ok_button);
        
	if (font_face[0])
	    cairo_select_font_face(c, font_face, font_slant, font_weight);
	else
	    cairo_select_font_face(c, "Arial",
				   CAIRO_FONT_SLANT_NORMAL,
				   CAIRO_FONT_WEIGHT_BOLD);

	// black rect on top
        cairo_set_source_rgb(c, 0.0, 0.0, 0.0);
        cairo_rectangle(c, 0.0, 0.0, width, FIRST_BLOCK_HEIGHT);
        cairo_fill(c);

        // white area
        cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
        if (white_first)
            cairo_rectangle(c, 0.0, SECOND_BLOCK_START, width, OTHER_BLOCK_HEIGHT);
        else
            cairo_rectangle(c, 0.0, THIRD_BLOCK_START, width, OTHER_BLOCK_HEIGHT);
        cairo_fill(c);

        if (blue_background())
            cairo_set_source_rgb(c, 0.0, 0.0, 1.0);
        else
            cairo_set_source_rgb(c, 1.0, 0.0, 0.0);

        // blue area
        if (white_first)
            cairo_rectangle(c, 0.0, THIRD_BLOCK_START, width, OTHER_BLOCK_HEIGHT);
        else
            cairo_rectangle(c, 0.0, SECOND_BLOCK_START, width, OTHER_BLOCK_HEIGHT);
        cairo_fill(c);

        // write category
	gdouble right = 0.0;
        cairo_set_font_size(c, 0.6*FIRST_BLOCK_HEIGHT);
	if (cat_round & ROUND_EXTRA_MATCH)
	    cairo_set_source_rgb(c, 1.0, 1.0, 0.0);
	else
	    cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
#if 0
        cairo_text_extents(c, category, &extents);
	right = 10.0 + extents.width;
        cairo_move_to(c, 10.0, (FIRST_BLOCK_HEIGHT - extents.height)/2.0 - extents.y_bearing);
        cairo_show_text(c, category);
#endif
	{
	    gdouble _x = 10.0, _y = 0, _w = 0, _h = FIRST_BLOCK_HEIGHT;
	    write_text(c, category, &_x, &_y, &_w, &_h,
		       TEXT_ANCHOR_START, TEXT_ANCHOR_MIDDLE, NULL,
		       0.6*FIRST_BLOCK_HEIGHT, 0);
	    right = 10.0 + _w;
	}

        // rest time
        if (rrest) {
            gchar buf[16];
            snprintf(buf, sizeof(buf), "%d:%d%d", rmin, rtsec, rsec);
            cairo_set_source_rgb(c, 1.0, 0, 0);
            cairo_text_extents(c, "88:88", &extents);
            cairo_move_to(c, width - 10.0 - extents.width,
                          (FIRST_BLOCK_HEIGHT - extents.height)/2.0 - extents.y_bearing);
            cairo_show_text(c, buf);

            if ((white_first && (rflags & MATCH_FLAG_BLUE_REST)) ||
                (white_first == FALSE && (rflags & MATCH_FLAG_WHITE_REST)))
                cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
            else if (blue_background())
                cairo_set_source_rgb(c, 0, 0, 1.0);
            else
                cairo_set_source_rgb(c, 1.0, 0, 0);

            if (rsec & 1) {
                cairo_rectangle(c, width - 20.0 - extents.width - extents.height,
                                (FIRST_BLOCK_HEIGHT - extents.height)/2.0,
                                extents.height, extents.height);
                cairo_fill(c);
            }
        } else {
	    cairo_set_font_size(c, 0.5*FIRST_BLOCK_HEIGHT);
	    const gchar *rnd = round_to_str(cat_round);
	    cairo_text_extents(c, rnd, &extents);
	    gdouble left = width - 10.0 - extents.width;
	    if (left < right + 10.0) left = right + 10.0;
	    cairo_move_to(c, left,
			  (FIRST_BLOCK_HEIGHT - extents.height)/2.0 - extents.y_bearing);
	    cairo_show_text(c, rnd);
	}

        name_start = 10.0;

        // flags
        if (showflags && b_country[0] && w_country[0]) {
            draw_flag(c, SECOND_BLOCK_START, OTHER_BLOCK_HEIGHT, b_country, 0.1*flagsize);
            draw_flag(c, THIRD_BLOCK_START, OTHER_BLOCK_HEIGHT, w_country, 0.1*flagsize);
        }

        // name of the first competitor
        if (white_first)
            cairo_set_source_rgb(c, 0.0, 0.0, 0.0);
        else
            cairo_set_source_rgb(c, 1.0, 1.0, 1.0);

/****
        cairo_set_font_size(c, namesize*0.06*OTHER_BLOCK_HEIGHT);
        cairo_text_extents(c, b_last, &extents);
        cairo_move_to(c, name_start+5.0, SECOND_BLOCK_START + (OTHER_BLOCK_HEIGHT - extents.height)/2.0 - extents.y_bearing);
        cairo_show_text(c, b_last);
****/
	{
	    gdouble _x = name_start+5.0, _y = SECOND_BLOCK_START, _w = 0, _h = OTHER_BLOCK_HEIGHT;
	    write_text(c, b_last, &_x, &_y, &_w, &_h,
		       TEXT_ALIGN_LEFT, TEXT_ALIGN_MIDDLE, NULL,
		       namesize*0.06*OTHER_BLOCK_HEIGHT, 0);
	}

        // name of the second competitor
        if (white_first)
            cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
        else
            cairo_set_source_rgb(c, 0.0, 0.0, 0.0);

        //cairo_text_extents(c, w_last, &extents);
        //cairo_move_to(c, name_start+5.0, THIRD_BLOCK_START + (OTHER_BLOCK_HEIGHT - extents.height)/2.0 - extents.y_bearing);
        //cairo_show_text(c, w_last);
	{
	    gdouble _x = name_start+5.0, _y = THIRD_BLOCK_START, _w = 0, _h = OTHER_BLOCK_HEIGHT;
	    write_text(c, w_last, &_x, &_y, &_w, &_h,
		       TEXT_ALIGN_LEFT, TEXT_ALIGN_MIDDLE, NULL,
		       namesize*0.06*OTHER_BLOCK_HEIGHT, 0);
	}

        // judogi control warning
        if (judogi_control) {
            static gboolean even = FALSE;
            if (even) {
                gchar buf[16];
                snprintf(buf, sizeof(buf), "%s", _("CTL"));
                cairo_set_source_rgb(c, 0, 0, 0);
                cairo_text_extents(c, buf, &extents);

                if (!(judogi_control_flags & MATCH_FLAG_JUDOGI1_OK) && white_ctl) {
                    gdouble scale = OTHER_BLOCK_HEIGHT/cairo_image_surface_get_height(white_ctl);
                    cairo_save(c);
                    cairo_scale(c, scale, scale);
                    cairo_set_source_surface(c, white_ctl, (width - OTHER_BLOCK_HEIGHT)/scale, SECOND_BLOCK_START/scale);
                    cairo_paint(c);
                    cairo_restore(c);
                }

                if (!(judogi_control_flags & MATCH_FLAG_JUDOGI2_OK) && blue_ctl) {
                    gdouble scale = OTHER_BLOCK_HEIGHT/cairo_image_surface_get_height(blue_ctl);
                    cairo_save(c);
                    cairo_scale(c, scale, scale);
                    cairo_set_source_surface(c, blue_ctl, (width - OTHER_BLOCK_HEIGHT)/scale, THIRD_BLOCK_START/scale);
                    cairo_paint(c);
                    cairo_restore(c);
                }
            }
            even = !even;
        }

        return FALSE;
    }

    //gtk_window_get_size(ad_window, &width, &height);

    gdouble scalex = 1.0*width/frame->width,
        scaley = 1.0*height/frame->height,
        scale = (scalex < scaley) ? scalex : scaley;

    gdouble x = (scale*frame->width < width) ? (width/scale - frame->width)/2.0 : 0;
    gdouble y = (scale*frame->height < height) ? (height/scale - frame->height)/2.0 : 0;

    cairo_t *c = (cairo_t *)event;

    cairo_set_source_rgb(c, 0.0, 0.0, 0.0);
    cairo_rectangle(c, 0.0, 0.0, width, height);
    cairo_fill(c);

    cairo_scale(c, scale, scale);

    cairo_set_source_surface(c, frame->surface, x, y);
    cairo_paint(c);

    return FALSE;
}

static gboolean delete_event_ad( GtkWidget *widget,
                                 GdkEvent  *event,
                                 gpointer   data )
{
    return FALSE;
}

static void destroy_ad( GtkWidget *widget,
                        gpointer   data )
{
    struct message msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_UPDATE_LABEL;
    msg.u.update_label.label_num = STOP_COMPETITORS;

    /*write_tv_logo(&(msg.u.update_label));*/

    if (mode != MODE_SLAVE)
        send_label_msg(&msg);

    ad_window = NULL;
    no_ads = FALSE;
    comp_names_pending = FALSE;
    comp_names_start = 0;
    ok_button = NULL;
    rrest = judogi_control = FALSE;
    rsec = 0;
}

static gboolean refresh_frame(gpointer data)
{
    GtkWidget *widget;

    if (ad_window == NULL)
        return FALSE;

    if (comp_names_pending && no_ads) {
#if 0
        if (comp_names_start == 0)
            comp_names_start = time(NULL);
        else if (time(NULL) - comp_names_start > 3) {
            no_ads = FALSE;
            comp_names_pending = FALSE;
            comp_names_start = 0;
            gtk_widget_destroy(GTK_WIDGET(data));
            return FALSE;

        }
#endif
        return TRUE;
    }

    current_frame++;
    if (get_current_frame(GTK_WIDGET(data)) == NULL) {
        current_frame = 0;
        current_ad++;
        if (current_ad >= num_ads)
            current_ad = 0;

        if (comp_names_pending) {
            no_ads = TRUE;
            goto update;
        }

        gtk_widget_destroy(GTK_WIDGET(data));
        return FALSE;
    }

 update:
    widget = GTK_WIDGET(data);
    gtk_widget_queue_draw(GTK_WIDGET(widget));
    return TRUE;
}

static void save_advertisement(GifRowType *ScreenBuffer,
			       gint swidth, gint sheight, gint x, gint y,
			       gint width, gint height)
{
    gint i, j;
    GifRowType GifRow;
    GifColorType *ColorMapEntry;
    guchar *data, *p;

    struct frame *frame = g_malloc(sizeof(*frame));
    frame->next = NULL;
    frame->x = 0;//x;
    frame->y = 0;//y;
    frame->width = swidth;
    frame->height = sheight;
    frame->stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, swidth);
    data = g_malloc(frame->stride*sheight);

    //mylog("\nsave frame=%p x=%d y=%d\n", frame, x, y);

    for (i = 0; i < sheight; i++) {
        GifRow = ScreenBuffer[i];
        p = &data[frame->stride*i];

        for (j = 0; j < swidth; j++) {
            ColorMapEntry = &ColorMap->Colors[GifRow[j]];
            *p++ = ColorMapEntry->Blue;
            *p++ = ColorMapEntry->Green;
            *p++ = ColorMapEntry->Red;
            *p++ = 0;
        }
    }

    frame->surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_RGB24,
                                                         swidth, sheight, frame->stride);
    advertisements[num_ads].swidth = swidth;
    advertisements[num_ads].sheight = sheight;

    //mylog("install num_ads=%d:", num_ads);
    struct frame *f = &advertisements[num_ads].frames;

    while (f->next) {
        //mylog(" f=%p->%p", f, f->next);
        f = f->next;
    }

    f->next = frame;
}

extern GtkWidget *main_window;

//#define USE_FULL_SCREEN 1

static gboolean close_display(GtkWidget *widget, GdkEventKey *event, gpointer userdata)
{
    if (event && event->type == GDK_KEY_PRESS) {
        if (event->keyval == GDK_Left || event->keyval == GDK_BackSpace) {
            if (ad_svg_handle.current_page > 0) ad_svg_handle.current_page--;
            if (mode != MODE_SLAVE) {
                struct message msg;
                memset(&msg, 0, sizeof(msg));
                msg.type = MSG_UPDATE_LABEL;
                msg.u.update_label.label_num = STOP_COMPETITORS;
                msg.u.update_label.xalign = 0xff00 | ad_svg_handle.current_page;
                send_label_msg(&msg);
            }
            gtk_widget_queue_draw(GTK_WIDGET(ad_window));
            return FALSE;
        }

        ad_svg_handle.current_page++;
        if ((1 << ad_svg_handle.current_page) & ad_svg_handle.page_mask) {
            if (mode != MODE_SLAVE) {
                struct message msg;
                memset(&msg, 0, sizeof(msg));
                msg.type = MSG_UPDATE_LABEL;
                msg.u.update_label.label_num = STOP_COMPETITORS;
                msg.u.update_label.xalign = 0xff00 | ad_svg_handle.current_page;
                send_label_msg(&msg);
            }
            gtk_widget_queue_draw(GTK_WIDGET(ad_window));
            return FALSE;
        }
    }

    ad_svg_handle.current_page = 0;
    
    if (!fullscreen) {
        gtk_window_get_position(GTK_WINDOW(userdata), &adwin_x, &adwin_y);
        gtk_window_get_size(GTK_WINDOW(userdata), &adwin_w, &adwin_h);
        g_key_file_set_integer(keyfile, "preferences", "adwin_x", adwin_x);
        g_key_file_set_integer(keyfile, "preferences", "adwin_y", adwin_y);
        g_key_file_set_integer(keyfile, "preferences", "adwin_w", adwin_w);
        g_key_file_set_integer(keyfile, "preferences", "adwin_h", adwin_h);
    }
    
    gtk_widget_destroy(userdata);
    return FALSE;
}

static gboolean close_display_2(GtkWidget *widget, gpointer userdata)
{
    return close_display(widget, NULL, userdata);
}

void close_ad_window(gint pagenum)
{
    if (ad_window) {
        if (pagenum) {
            ad_svg_handle.current_page = pagenum & 0xff;
            gtk_widget_queue_draw(GTK_WIDGET(ad_window));
        } else {
            close_display(NULL, NULL, ad_window);
        }
    }
}

void display_ad_window(void)
{
    if (ad_window)
        return;

    if (comp_names_pending == FALSE && num_ads == 0)
        return;

    clear_data();
    
    gint width;
    gint height;
    gtk_window_get_size(GTK_WINDOW(main_window), &width, &height);

    GtkWindow *window = ad_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    gtk_window_set_keep_above(GTK_WINDOW(ad_window), TRUE);
    //gtk_window_set_title(GTK_WINDOW(window), _("Advertisement"));

    if (fullscreen)
        gtk_window_fullscreen(GTK_WINDOW(window));
    else if (adwin_w > 100 && adwin_h > 100) {
        gtk_window_set_default_size(GTK_WINDOW(window), adwin_w, adwin_h);
        gtk_window_move(GTK_WINDOW(window), adwin_x, adwin_y);
    } else {
        gtk_window_set_default_size(GTK_WINDOW(window), width, height);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    }
    
    gtk_window_set_modal(GTK_WINDOW(window),TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(window),GTK_WINDOW(main_window));

    GtkWidget *vbox;
    vbox = gtk_grid_new();
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 1);
    if (mode != MODE_SLAVE) {
        ok_button = gtk_button_new_with_label(_("OK"));
        gtk_grid_attach(GTK_GRID(vbox), ok_button, 0, 0, 1, 1);
    } else
        ok_button = NULL;

    GtkWidget *darea = gtk_drawing_area_new();
    gtk_grid_attach(GTK_GRID(vbox), darea, 0, 1, 1, 1);
    gtk_widget_set_hexpand(darea, TRUE);
    gtk_widget_set_vexpand(darea, TRUE);

    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show_all(GTK_WIDGET(window));

    ad_svg_handle.current_page = 0;
    
    if (ok_button)
        gtk_widget_hide(ok_button);

    g_signal_connect (G_OBJECT (window), "delete_event",
                      G_CALLBACK (delete_event_ad), NULL);
    g_signal_connect (G_OBJECT (window), "destroy",
                      G_CALLBACK (destroy_ad), NULL);
    g_signal_connect(G_OBJECT(darea),
                     "draw", G_CALLBACK(expose_ad), NULL);

    gtk_widget_add_events(darea, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(darea),
                     "button-press-event", G_CALLBACK(mouse_click), NULL);

    g_signal_connect(G_OBJECT(window),
                     "key-press-event", G_CALLBACK(close_display), window);

    if (ok_button)
        g_signal_connect(G_OBJECT(ok_button),
                         "clicked", G_CALLBACK(close_display_2), window);

    if (comp_names_pending) {
        //g_timeout_add(mode == MODE_SLAVE ? 2000 : 100000, refresh_frame, window);
    } else if (current_ad_has_one_frame())
        g_timeout_add(2000, refresh_frame, window);
    else
        g_timeout_add(100, refresh_frame, window);
}


static gchar *ad_directory = NULL;

void toggle_show_comp(GtkWidget *menu_item, gpointer data)
{
    GtkWidget *dialog, *show_flags, *show_letter, *show_names, *vbox, *label;
    GtkWidget *svgfile;

    dialog = gtk_dialog_new_with_buttons (_("Show Competitors"),
                                          GTK_WINDOW(main_window),
                                          GTK_DIALOG_MODAL,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                                          NULL);

    vbox = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(vbox), TRUE);
    //gtk_grid_set_row_spacing(GTK_GRID(vbox), 5);

    show_names   = gtk_check_button_new_with_label(_("Show Competitors"));
    show_flags   = gtk_check_button_new_with_label(_("Show Flags"));
    show_letter  = gtk_check_button_new_with_label(_("Show Initial of Name"));

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_names), show_competitor_names);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_flags), showflags);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_letter), showletter);

    /*
    GtkAdjustment *flag_adj = gtk_adjustment_new(flagsize, 3.0, 11.0, 1.0, 1.0, 1.0);
    GtkWidget *flag_scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT (flag_adj));
    gtk_scale_set_digits(GTK_SCALE(flag_scale), 1);
    gtk_scale_set_value_pos(GTK_SCALE(flag_scale), GTK_POS_TOP);
    gtk_scale_set_draw_value(GTK_SCALE(flag_scale), TRUE);
    */
    GtkWidget *flag_scale = gtk_spin_button_new_with_range(3.0, 10.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(flag_scale), flagsize);

    /*
    GtkAdjustment *name_adj = gtk_adjustment_new(namesize, 3.0, 11.0, 1.0, 1.0, 1.0);
    GtkWidget *name_scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT (name_adj));
    gtk_scale_set_digits(GTK_SCALE(name_scale), 1);
    gtk_scale_set_value_pos(GTK_SCALE(name_scale), GTK_POS_TOP);
    gtk_scale_set_draw_value(GTK_SCALE(name_scale), TRUE);
    */
    GtkWidget *name_scale = gtk_spin_button_new_with_range(3.0, 10.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(name_scale), namesize);

    gtk_grid_attach(GTK_GRID(vbox), show_names,  0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(vbox), show_flags,  0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(vbox), show_letter, 1, 2, 1, 1);

    label = gtk_label_new(_("Flag size:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
    gtk_grid_attach(GTK_GRID(vbox), label,  0, 3, 1, 1);

    gtk_grid_attach(GTK_GRID(vbox), flag_scale,  1, 3, 1, 1);

    label = gtk_label_new(_("Name size:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
    gtk_grid_attach(GTK_GRID(vbox), label,  0, 4, 1, 1);

    gtk_grid_attach(GTK_GRID(vbox), name_scale,  1, 4, 1, 1);

    // SVG
    svgfile = gtk_button_new_with_label(_("SVG Templates"));
    g_signal_connect(G_OBJECT(svgfile), "button-press-event", G_CALLBACK(set_svg_file), 0);
    gtk_grid_attach(GTK_GRID(vbox), svgfile,  0, 5, 1, 1);
    
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       vbox, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        show_competitor_names = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_names));
        showflags = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_flags));
        showletter = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_letter));
        /*
        flagsize = gtk_adjustment_get_value(GTK_ADJUSTMENT(flag_adj));
        namesize = gtk_adjustment_get_value(GTK_ADJUSTMENT(name_adj));
        */
        flagsize = gtk_spin_button_get_value(GTK_SPIN_BUTTON(flag_scale));
        namesize = gtk_spin_button_get_value(GTK_SPIN_BUTTON(name_scale));

        g_key_file_set_boolean(keyfile, "preferences", "showcompetitornames", show_competitor_names);
        g_key_file_set_boolean(keyfile, "preferences", "showflags", showflags);
        g_key_file_set_boolean(keyfile, "preferences", "showletter", showletter);
        g_key_file_set_double(keyfile, "preferences", "flagsize", flagsize);
        g_key_file_set_double(keyfile, "preferences", "namesize", namesize);
    }

    gtk_widget_destroy(dialog);
}

void toggle_advertise(GtkWidget *menu_item, gpointer data)
{
    GtkWidget *dialog, *do_ads;
    gboolean ok;

    dialog = gtk_file_chooser_dialog_new(_("Choose a directory"),
                                         NULL,
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);

    do_ads = gtk_check_button_new_with_label(_("Run Advertisements"));
    //gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(do_ads), TRUE);
    gtk_widget_show(do_ads);

    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), do_ads);

    if (ad_directory)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            ad_directory);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(dialog);
        return;
    }

    ok = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(do_ads));

    if (ok) {
        g_free(ad_directory);
        ad_directory = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }

    gtk_widget_destroy (dialog);

    num_ads = 0;
    current_ad = 0;
    current_frame = 0;
    memset(&advertisements, 0, sizeof(advertisements));

    if (ok) {
        GDir *dir = g_dir_open(ad_directory, 0, NULL);
        if (dir) {
            const gchar *fname = g_dir_read_name(dir);
            while (fname) {
                gchar *fullname = g_build_filename(ad_directory, fname, NULL);
                if (strstr(fname, ".gif"))
                    read_gif(fullname);
                else if (strstr(fname, ".png") && num_ads < NUM_ADS) {
                    struct frame *frame = g_malloc(sizeof(*frame));
                    memset(frame, 0, sizeof(*frame));
                    frame->surface = cairo_image_surface_create_from_png(fullname);
                    frame->width = cairo_image_surface_get_width(frame->surface);
                    frame->height = cairo_image_surface_get_height(frame->surface);
                    advertisements[num_ads].frames.next = frame;
                    num_ads++;
                }
                g_free(fullname);
                fname = g_dir_read_name(dir);
            }
            g_dir_close(dir);
        }
    }
}

void display_comp_window(gchar *cat, gchar *comp1, gchar *comp2,
                         gchar *first1, gchar *first2,
                         gchar *country1, gchar *country2,
                         gchar *club1, gchar *club2, gint round)
{
    if (!show_competitor_names)
        return;

    STRCPY_UTF8(category, cat);
    cat_round = round;

    b_last[0] = w_last[0] = 0;
    b_first[0] = w_first[0] = 0;
    b_country[0] = w_country[0] = 0;
    b_club[0] = w_club[0] = 0;

    parse_name(comp1, b_first, b_last, b_club, b_country);
    parse_name(comp2, w_first, w_last, w_club, w_country);
#if 0    
    // check if all data is in names
    STRCPY_UTF8(b_last, comp1);
    STRCPY_UTF8(w_last, comp2);

    p = strchr(b_last, '\t');
    if (p) {
        *p = 0;
        STRCPY_UTF8(b_first, p+1);
        p = strchr(b_first, '\t');
        if (p) {
            *p = 0;
            STRCPY_UTF8(b_country, p+1);
            p = strchr(b_country, '\t');
            if (p) *p = 0;
        }
    }

    p = strchr(w_last, '\t');
    if (p) {
        *p = 0;
        STRCPY_UTF8(w_first, p+1);
        p = strchr(w_first, '\t');
        if (p) {
            *p = 0;
            STRCPY_UTF8(w_country, p+1);
            p = strchr(w_country, '\t');
            if (p) *p = 0;
        }
    }

#endif

    if (first1 && first1[0])
        STRCPY_UTF8(b_first, first1);
    if (first2 && first2[0])
        STRCPY_UTF8(w_first, first2);

    if (country1 && country1[0])
        STRCPY_UTF8(b_country, country1);
    if (country2 && country2[0])
        STRCPY_UTF8(w_country, country2);

    if (club1 && club1[0])
        STRCPY_UTF8(b_club, club1);
    if (club2 && club2[0])
        STRCPY_UTF8(w_club, club2);
    
    if (showletter) {
        gchar buf[8], buf2[32];
        if (b_first[0]) {
            g_utf8_strncpy(buf, b_first, 1);
            SNPRINTF_UTF8(buf2, "%s.%s", buf, b_last);
            STRCPY_UTF8(b_last, buf2);
        }
        if (w_first[0]) {
            g_utf8_strncpy(buf, w_first, 1);
            SNPRINTF_UTF8(buf2, "%s.%s", buf, w_last);
            STRCPY_UTF8(w_last, buf2);
        }
    }

    comp_names_start = 0;
    comp_names_pending = TRUE;
    if (ad_window == NULL)
        no_ads = TRUE;
    display_ad_window();
}

/*
 * SVG
 */

static gint svg_img_cb(svg_handle *svg)
{
    if (svg->images[svg->current_page][IMAGE_FLAG1].exists)
        draw_flag_common(svg, &svg->images[svg->current_page][IMAGE_FLAG1].rect, b_country);
    if (svg->images[svg->current_page][IMAGE_FLAG2].exists)
        draw_flag_common(svg, &svg->images[svg->current_page][IMAGE_FLAG2].rect, w_country);

    if (svg->images[svg->current_page][IMAGE_PREVFLAG1].exists)
        draw_flag_common(svg, &svg->images[svg->current_page][IMAGE_PREVFLAG1].rect, prevteam1country);
    if (svg->images[svg->current_page][IMAGE_PREVFLAG2].exists)
        draw_flag_common(svg, &svg->images[svg->current_page][IMAGE_PREVFLAG2].rect, prevteam2country);

    if (svg->images[svg->current_page][IMAGE_NEXTFLAG1].exists)
        draw_flag_common(svg, &svg->images[svg->current_page][IMAGE_NEXTFLAG1].rect, nextteam1country);
    if (svg->images[svg->current_page][IMAGE_NEXTFLAG2].exists)
        draw_flag_common(svg, &svg->images[svg->current_page][IMAGE_NEXTFLAG2].rect, nextteam2country);
    
    if (judogi_control) {
        if (svg->images[svg->current_page][IMAGE_CONTROL1].exists && !(judogi_control_flags & MATCH_FLAG_JUDOGI1_OK) && white_ctl)
            draw_image_common(svg, &svg->images[svg->current_page][IMAGE_CONTROL1].rect, white_ctl, TRUE);
        if (svg->images[svg->current_page][IMAGE_CONTROL2].exists && !(judogi_control_flags & MATCH_FLAG_JUDOGI2_OK) && blue_ctl)
            draw_image_common(svg, &svg->images[svg->current_page][IMAGE_CONTROL2].rect, blue_ctl, TRUE);
    }
    return 0;
}

static void clear_data(void)
{
    if (prevteam1name) g_free(prevteam1name);
    if (prevteam2name) g_free(prevteam2name);
    if (nextteam1name) g_free(nextteam1name);
    if (nextteam2name) g_free(nextteam2name);
    if (prevteam1country) g_free(prevteam1country);
    if (prevteam2country) g_free(prevteam2country);
    if (nextteam1country) g_free(nextteam1country);
    if (nextteam2country) g_free(nextteam2country);
    prevteam1name = prevteam2name = 0;
    nextteam1name = nextteam2name = 0;
    prevteam1country = prevteam2country = 0;
    nextteam1country = nextteam2country = 0;
    team1wins = team2wins = 0;
    team1pts = team2pts = 0;

    free_svg_memory_common(&svg_reply);
}

static gboolean get_json(void)
{
    if (svg_reply.response == NULL) {
        gchar buf[256];

        clear_data();

        if (last_reported_category == 0)
            last_reported_category = current_category;
        
        snprintf(buf, sizeof(buf),
                 "{\"op\":\"sql\", \"cmd\":\""
                 "select c1.last,c2.last,c1.country,c2.country from matches as m "
                 "left join competitors as c1 on m.blue=c1.'index' "
                 "left join competitors as c2 on m.white=c2.'index' "
                 "where m.category=%d and m.number=1\"}",
                 last_reported_category & 0xffff);
        ask_json_common(buf, &svg_reply);
        prevteam1name = g_strdup(get_array_2_str_common(svg_reply.json, 1, 0));
        prevteam2name = g_strdup(get_array_2_str_common(svg_reply.json, 1, 1));
        prevteam1country = g_strdup(get_array_2_str_common(svg_reply.json, 1, 2));
        prevteam2country = g_strdup(get_array_2_str_common(svg_reply.json, 1, 3));

        free_svg_memory_common(&svg_reply);
        
        snprintf(buf, sizeof(buf),
                 "{\"op\":\"sql\", \"cmd\":\""
                 "select c1.last,c2.last,c1.country,c2.country from matches as m "
                 "left join competitors as c1 on m.blue=c1.'index' "
                 "left join competitors as c2 on m.white=c2.'index' "
                 "where m.category=%d and m.number=1\"}",
                 current_category & 0xffff);
        ask_json_common(buf, &svg_reply);
        nextteam1name = g_strdup(get_array_2_str_common(svg_reply.json, 1, 0));
        nextteam2name = g_strdup(get_array_2_str_common(svg_reply.json, 1, 1));
        nextteam1country = g_strdup(get_array_2_str_common(svg_reply.json, 1, 2));
        nextteam2country = g_strdup(get_array_2_str_common(svg_reply.json, 1, 3));

        free_svg_memory_common(&svg_reply);
        
        snprintf(buf, sizeof(buf),
                 "{\"op\":\"sql\", \"cmd\":\""
                 "select m.blue_points,m.white_points,c1.last,c2.last from matches as m "
                 "left join competitors as c1 on m.blue=c1.'index' "
                 "left join competitors as c2 on m.white=c2.'index' "
                 "where m.category=%d\"}",
                 last_reported_category);
        ask_json_common(buf, &svg_reply);

        struct cJSON *lst = get_array_1_common(svg_reply.json, 1);
        if (!lst) return FALSE;
        while (lst) {
            gint val1 = 0, val2 = 0;
            struct cJSON *v;
            v = cJSON_GetArrayItem(lst, 0);
            if (v) val1 = atoi(v->valuestring);
            v = cJSON_GetArrayItem(lst, 1);
            if (v) val2 = atoi(v->valuestring);

            if (val1 > val2) team1wins++;
            else if (val1 < val2) team2wins++;
            
            if (val1 == 7) val1 = 1;
            if (val2 == 7) val2 = 1;
            team1pts += val1;
            team2pts += val2;
            
            lst = lst->next;
        }
    }

    return svg_reply.json != NULL;
}

static gint svg_attr_cb(svg_handle *svg)
{
    //mylog("ATTR=%s\n", svg->attr[0].code);
    if (svg->attr[0].code[0] == 'R') {
        const gchar *rnd = round_to_str(cat_round);
        WRITE(rnd);
    } else if (svg->attr[0].code[0] == 'C') {
        WRITE(category);
    } else if (IS_SAME(svg->attr[0].code, "last")) {
        gint compnum = svg->attr[0].value;
        if (compnum == 1)
            WRITE(b_last);
        else
            WRITE(w_last);
    } else if (IS_SAME(svg->attr[0].code, "first")) {
        gint compnum = svg->attr[0].value;
        if (compnum == 1)
            WRITE(b_first);
        else
            WRITE(w_first);
    } else if (IS_SAME(svg->attr[0].code, "f")) {
        gchar buf[8];
        gint compnum = svg->attr[0].value;
        if (compnum == 1)
            g_utf8_strncpy(buf, b_first, 1);
        else
            g_utf8_strncpy(buf, w_first, 1);
        WRITE(buf);
    } else if (IS_SAME(svg->attr[0].code, "country")) {
        gint compnum = svg->attr[0].value;
        if (compnum == 1)
            WRITE(b_country);
        else
            WRITE(w_country);
    } else if (IS_SAME(svg->attr[0].code, "club")) {
        gint compnum = svg->attr[0].value;
        if (compnum == 1)
            WRITE(b_club);
        else
            WRITE(w_club);
    } else if (IS_SAME(svg->attr[0].code, "rest")) {
        gint who = svg->attr[0].value;
        if (rrest && who == 1 && (rflags & MATCH_FLAG_BLUE_REST)) {
            gchar buf[16];
            snprintf(buf, sizeof(buf), "%d:%d%d", rmin, rtsec, rsec);
            WRITE(buf);
        }
        if (rrest && who == 2 && (rflags & MATCH_FLAG_WHITE_REST)) {
            gchar buf[16];
            snprintf(buf, sizeof(buf), "%d:%d%d", rmin, rtsec, rsec);
            WRITE(buf);
        }
    } else if (IS_SAME(svg->attr[0].code, "prevteam")) {
        if (!get_json()) return 0;
        gint who = svg->attr[0].value;
        if (who == 1 && prevteam1name) WRITE(prevteam1name);
        else if (who == 2 && prevteam2name) WRITE(prevteam2name);
    } else if (IS_SAME(svg->attr[0].code, "nextteam")) {
        if (!get_json()) return 0;
        gint who = svg->attr[0].value;
        if (who == 1 && nextteam1name) WRITE(nextteam1name);
        else if (who == 2 && nextteam2name) WRITE(nextteam2name);
    } else if (IS_SAME(svg->attr[0].code, "prevcountry")) {
        gint compnum = svg->attr[0].value;
        if (compnum == 1)
            WRITE(prevteam1country);
        else
            WRITE(prevteam2country);
    } else if (IS_SAME(svg->attr[0].code, "nextcountry")) {
        gint compnum = svg->attr[0].value;
        if (compnum == 1)
            WRITE(nextteam1country);
        else
            WRITE(nextteam2country);
    } else if (IS_SAME(svg->attr[0].code, "wins")) {
        if (!get_json()) return 0;
        gint who = svg->attr[0].value;
        gchar buf[16];
        if (who == 1) snprintf(buf, sizeof(buf), "%d", team1wins);
        else snprintf(buf, sizeof(buf), "%d", team2wins);
        WRITE(buf);
    } else if (IS_SAME(svg->attr[0].code, "pts")) {
        if (!get_json()) return 0;
        gint who = svg->attr[0].value;
        gchar buf[16];
        if (who == 1) snprintf(buf, sizeof(buf), "%d", team1pts);
        else snprintf(buf, sizeof(buf), "%d", team2pts);
        WRITE(buf);
    }
    return 0;
}

void read_svg_file(gchar *fname) // fname must be allocated and free'd
{
    if (!fname || fname[0] == 0) return;
    mylog("fname=%s\n", fname);
    ad_svg_handle.svg_ok = FALSE;
    ad_svg_handle.imagename = imagename;
    g_free(ad_svg_handle.svg_file);
    ad_svg_handle.svg_file = fname;
    read_svg_file_common(&ad_svg_handle, GTK_WINDOW(main_window));
    ad_svg_handle.svg_cb = svg_attr_cb;
    ad_svg_handle.img_cb = svg_img_cb;
}

void set_svg_file(GtkWidget *menu_item, GdkEventButton *event, gpointer data)
{
    gchar *fname = get_svg_file_name_common(GTK_WINDOW(main_window), keyfile, "adsvgfile");
    mylog("set_svg_file file=%s\n", fname);
    if (!fname) {
	ad_svg_handle.svg_ok = FALSE;
	return;
    }
    ad_svg_handle.svg_ok = TRUE;
    read_svg_file(fname);
}

static gboolean mouse_click(GtkWidget *window,
                            GdkEventButton *event,
                            gpointer userdata)
{
    if (ad_svg_handle.paper_width == 0 || ad_svg_handle.paper_height == 0 || ad_svg_handle.svg_ok == FALSE ||
        ad_svg_handle.images[ad_svg_handle.current_page][IMAGE_CLOSE].exists == FALSE)
        return FALSE;
    
    if (event->type == GDK_BUTTON_PRESS  &&
        (event->button == 1 || event->button == 3)) {
        gdouble x = event->x*ad_svg_handle.svg_width/ad_svg_handle.paper_width;
        gdouble y = event->y*ad_svg_handle.svg_height/ad_svg_handle.paper_height;
        RsvgRectangle *rect = &ad_svg_handle.images[ad_svg_handle.current_page][IMAGE_CLOSE].rect;

        if (x >= rect->x && x < rect->x + rect->width &&
            y >= rect->y && y < rect->y + rect->height) {
            close_ad_window(0);
            return TRUE;
        }
    }

    return FALSE;
}
