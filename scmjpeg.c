#include <stdlib.h>
#include <tiffio.h>
#include <string.h>
#include <stdio.h>

#include "err.h"

#define TIFFTAG_SCM_INDEX   0xFFB1
#define TIFFTAG_SCM_OFFSET  0xFFB2
#define TIFFTAG_SCM_MINIMUM 0xFFB3
#define TIFFTAG_SCM_MAXIMUM 0xFFB4

static void extensions(TIFF *T)
{
    static const TIFFFieldInfo fields[] = {
        { TIFFTAG_SCM_INDEX,   TIFF_VARIABLE, TIFF_VARIABLE, TIFF_LONG8, FIELD_CUSTOM, 1, 1, "SCMIndex"   },
        { TIFFTAG_SCM_OFFSET,  TIFF_VARIABLE, TIFF_VARIABLE, TIFF_LONG8, FIELD_CUSTOM, 1, 1, "SCMOffset"  },
        { TIFFTAG_SCM_MINIMUM, TIFF_VARIABLE, TIFF_VARIABLE, TIFF_SBYTE, FIELD_CUSTOM, 1, 1, "SCMMinimum" },
        { TIFFTAG_SCM_MAXIMUM, TIFF_VARIABLE, TIFF_VARIABLE, TIFF_SBYTE, FIELD_CUSTOM, 1, 1, "SCMMaximum" },
    };

    TIFFMergeFieldInfo(T, fields, 4);
}

void copy16(TIFF *S, TIFF *D, ttag_t tag)
{
    uint16 v = 0;

    if (TIFFGetField(S, tag, &v))
        TIFFSetField(D, tag, v);
}

void copy32(TIFF *S, TIFF *D, ttag_t tag)
{
    uint32 v = 0;

    if (TIFFGetField(S, tag, &v))
        TIFFSetField(D, tag, v);
}

void copypv(TIFF *S, TIFF *D, ttag_t tag)
{
    uint64  n = 0;
    void   *p = 0;

    if (TIFFGetField(S, tag, &n, &p))
        TIFFSetField(D, tag,  n,  p);
}

int proc(const char *src, const char *dst)
{
    void   *ptr = NULL;
    tsize_t len = 0;

    TIFF *S = NULL;
    TIFF *D = NULL;
    int   N = 0;

    if ((S = TIFFOpen(src, "r8")) && (D = TIFFOpen(dst, "w8")))
    {
        // Inform LibTIFF of the SCM private fields.

        extensions(S);
        extensions(D);

        // Copy all directories from source to destination.
        do
        {
            // Set the destination compression to JPEG.

            TIFFSetField(D, TIFFTAG_COMPRESSION, 7);
            TIFFSetField(D, TIFFTAG_JPEGQUALITY, 85);

            // Copy all common tags.

            copy32(S, D, TIFFTAG_IMAGEWIDTH);
            copy32(S, D, TIFFTAG_IMAGELENGTH);
            copy32(S, D, TIFFTAG_ROWSPERSTRIP);
            copy16(S, D, TIFFTAG_BITSPERSAMPLE);
            copy16(S, D, TIFFTAG_SAMPLEFORMAT);
            copy16(S, D, TIFFTAG_SAMPLESPERPIXEL);
            copy16(S, D, TIFFTAG_ORIENTATION);
            copy16(S, D, TIFFTAG_PLANARCONFIG);
            copy16(S, D, TIFFTAG_PHOTOMETRIC);
            copypv(S, D, TIFFTAG_IMAGEDESCRIPTION);
            copypv(S, D, TIFFTAG_SCM_INDEX);
            copypv(S, D, TIFFTAG_SCM_OFFSET);
            copypv(S, D, TIFFTAG_SCM_MINIMUM);
            copypv(S, D, TIFFTAG_SCM_MAXIMUM);

            // Grow the strip buffer if necessary.

            tsize_t ns = TIFFNumberOfStrips(S);
            tsize_t ss = TIFFStripSize     (S);

            if (len < ss)
            {
                if ((ptr = realloc(ptr, ss)))
                    len = ss;
                else
                    syserr("Failed to allocate strip buffer size %d", ss);
            }

            // Copy and re-encode each strip.

            tsize_t i;

            for (i = 0; i < ns; ++i)
            {
                if ((ss = TIFFReadEncodedStrip (S, i, (tdata_t) ptr, -1)) == -1)
                    memset(ptr, 0, (ss = len));

                if ((ss = TIFFWriteEncodedStrip(D, i, (tdata_t) ptr, ss)) == -1)
                    apperr("Failed to write strip");
            }

            N++;

            TIFFFileno(D);
        }
        while (TIFFWriteDirectory(D) && TIFFReadDirectory(S));

        // Rebuild the offset table: Read it from the 0th directory, scan the
        // file updating the offset of each entry, and write it back.

        uint64  n = 0;
        uint64 *p = 0;

        if (TIFFSetDirectory(D, 0) &&
            TIFFGetField(D, TIFFTAG_SCM_OFFSET, &n, &p))
        {
            for (int i = 1; i < N; i++)
                if (TIFFSetDirectory(D, i))
                    p[i - 1] = TIFFCurrentDirOffset(D);

            if (TIFFSetDirectory(D, 0))
                TIFFSetField(D, TIFFTAG_SCM_OFFSET, n, p);
        }

        TIFFClose(S);
        TIFFClose(D);
    }
    return 0;
}

int main(int argc, char **argv)
{
    TIFFSetWarningHandler(0);

    if (argc == 3)
        return proc(argv[1], argv[2]);
    else
        return 1;
}
