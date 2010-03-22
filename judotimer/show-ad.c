/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2010 by Hannu Jokinen
 * Full copyright text is included in the software package.
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

#include "judotimer.h"

//#define T g_print("Error on line %d\n", __LINE__)
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
        g_print("\n[%d] GIF-LIB error: %s.\n", line, Err);
    else
        g_print("\n[%d] GIF-LIB undefined error %d.\n", line, _GifError);
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
    int	i, j, Size, Row, Col, Width, Height, ExtCode, Count;
    GifRecordType RecordType;
    GifByteType *Extension;
    GifRowType *ScreenBuffer;
    GifFileType *GifFile;

    //g_print("FILE %s\n", FileName);

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
            printf("\n\nleft=%d width=%d swidth=%d\ntop=%d height=%d sheight=%d\n\n",
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
                for (Count = i = 0; i < 4; i++)
                    for (j = Row + InterlacedOffset[i]; j < Row + Height;
                         j += InterlacedJumps[i]) {
                        //GifQprintf("\b\b\b\b%-4d", Count++);
                        if (DGifGetLine(GifFile, &ScreenBuffer[j][Col],
                                        Width) == GIF_ERROR) {
                            PrintGifError();
                            return(EXIT_FAILURE);
                        }
                    }
            }
            else {
                for (i = 0; i < Height; i++) {
                    //GifQprintf("\b\b\b\b%-4d", i);
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

static GtkWindow *ad_window = NULL;
                      
static struct frame *get_current_frame(void)
{
    gint i;
    struct frame *f = advertisements[current_ad].frames.next;

#if 0
    if (f && f->next == NULL && current_frame < 20)
        return f;
#endif
    for (i = 0; i < current_frame; i++) {
        if (!f)
            return NULL;
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

static gboolean expose_ad(GtkWidget *widget, GdkEventExpose *event, gpointer userdata)
{
    gint width, height;
    struct frame *frame = get_current_frame();
    if (!frame)
        return FALSE;

    width = widget->allocation.width;
    height = widget->allocation.height;
    //gtk_window_get_size(ad_window, &width, &height);

    gdouble scalex = 1.0*width/frame->width, 
        scaley = 1.0*height/frame->height,
        scale = (scalex < scaley) ? scalex : scaley;

    gdouble x = (scale*frame->width < width) ? (width/scale - frame->width)/2.0 : 0;
    gdouble y = (scale*frame->height < height) ? (height/scale - frame->height)/2.0 : 0;

    cairo_t *c = gdk_cairo_create(widget->window);

    cairo_set_source_rgb(c, 0.0, 0.0, 0.0);
    cairo_rectangle(c, 0.0, 0.0, width, height);
    cairo_fill(c);

    cairo_scale(c, scale, scale);

    cairo_set_source_surface(c, frame->surface, x, y);
    cairo_paint(c);

    cairo_show_page(c);
    cairo_destroy(c);

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
    ad_window = NULL;
}

static gboolean refresh_frame(gpointer data)
{
    GtkWidget *widget;
    GdkRegion *region;

    if (ad_window == NULL)
        return FALSE;

    current_frame++;
    if (get_current_frame() == NULL) {
        current_frame = 0;
        current_ad++;
        if (current_ad >= num_ads)
            current_ad = 0;
        gtk_widget_destroy(GTK_WIDGET(data));
        return FALSE;
    }

    widget = GTK_WIDGET(data);
    if (widget->window) {
        region = gdk_drawable_get_clip_region(widget->window);
        gdk_window_invalidate_region(widget->window, region, TRUE);
        gdk_window_process_updates(widget->window, TRUE);
    }
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

    //g_print("\nsave frame=%p x=%d y=%d\n", frame, x, y);

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
	
    //g_print("install num_ads=%d:", num_ads);
    struct frame *f = &advertisements[num_ads].frames;
		
    while (f->next) {
        //g_print(" f=%p->%p", f, f->next);
        f = f->next;
    }

    f->next = frame;
}

extern GtkWidget *main_window;

void display_ad_window(void)
{
    if (ad_window || num_ads == 0)
        return;

    gint width = advertisements[current_ad].swidth;
    gint height = advertisements[current_ad].sheight;

    gtk_window_get_size(GTK_WINDOW(main_window), &width, &height);

    GtkWindow *window = ad_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    gtk_window_set_title(GTK_WINDOW(window), _("Advertisement"));
    gtk_window_fullscreen(GTK_WINDOW(window));
    //gtk_widget_set_size_request(window, width, height);
    //gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *darea = gtk_drawing_area_new();
    gtk_container_add (GTK_CONTAINER (window), darea);
    gtk_widget_show_all(GTK_WIDGET(window));

    g_signal_connect (G_OBJECT (window), "delete_event",
                      G_CALLBACK (delete_event_ad), NULL);
    g_signal_connect (G_OBJECT (window), "destroy",
                      G_CALLBACK (destroy_ad), NULL);
    g_signal_connect(G_OBJECT(darea), 
                     "expose-event", G_CALLBACK(expose_ad), NULL);

    if (current_ad_has_one_frame())
        g_timeout_add(2000, refresh_frame, window);
    else
        g_timeout_add(100, refresh_frame, window);
}

static gchar *ad_directory = NULL;

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
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(do_ads), TRUE);
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

    if (!ok)
        return;

    GDir *dir = g_dir_open(ad_directory, 0, NULL);
    if (dir) {
        const gchar *fname = g_dir_read_name(dir);
        while (fname) {
            gchar *fullname = g_build_filename(ad_directory, fname, NULL);
            if (strstr(fname, ".gif"))
                read_gif(fullname);
            else if (strstr(fname, ".png") && num_ads < NUM_ADS) {
                struct frame *frame = g_malloc(sizeof(*frame));
                memset(frame, 0, sizeof(frame));
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
