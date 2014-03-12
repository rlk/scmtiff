# SCM TIFF

Spherical Cube Map processing utility.

## File Structure and Rationale

An SCM TIFF is a standard BigTIFF with a few private tags included. It can be read or written by LibTIFF version 4 and above. As of today, there are only a few tools that fully support BigTIFF, but the number of applications capable of manipulating SCM TIFF files is slowly growing.

### Paging

Each page in an SCM heirarchy is represented by a separate TIFF directory.

The 0th directory contains no data, as 0 is not a valid SCM page index. SCM TIFF uses the 0th directory to give global image parameters, including width, height, sample count, and sample format, which are guaranteed to be the same for all other directories in the file.

### Private tags

The private TIFF tags give global information that describe the set of pages appearing in the SCM hierarchy. The presense of these fields permits an application to make random accesses into heterogeneous image hierarchies in O(log n) time. Private fields appear only in the 0th directory of the image.

The private tags are:

- `TIFFTAG_SCM_INDEX 0xFFB1`

    This field gives a sorted listing of the SCM indices of all present pages. Each index is a 64-bit unsigned value, a `TIFF_LONG8`. A binary search of this field indicates whether the page with a given index does or does not appear in this file.

- `TIFFTAG_SCM_OFFSET 0xFFB2`

    This field gives the offset of each directory. There is a one-to-one mapping between entries in the `INDEX` field and the `OFFSET` field. This allows an application to seek directly to a page of data given the result of the search of the SCM index field.

- `TIFFTAG_SCM_MINIMUM 0xFFB3`

    This field gives the minimum value of a page. There is a one-to-one mapping between entries in the `INDEX` field and `MINIMUM` field.

- `TIFFTAG_SCM_MAXIMUM 0xFFB4`

    This field gives the maximum value of a page. There is a one-to-one mapping between entries in the `INDEX` field and `MAXIMUM` field.

Note, it is possible that the `OFFSET` entry for a page is zero. This indicates that the data of the page is *not* given by the file, but that the `MINIMUM` and `MAXIMUM` are given. The ability to query the bounds of a page without data affords applications a finer granularity of visibility determination. It is for this reason that the `MINIMUM` and `MAXIMUM` field are necessary, and simple reliance upon the TIFF standard `MinSampleValue 0x119` and `MaxSampleValue 0x118` does not suffice.
