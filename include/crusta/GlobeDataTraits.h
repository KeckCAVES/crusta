#ifndef _GlobeDataTraits_H_
#define _GlobeDataTraits_H_


#include <crusta/QuadtreeFile.h>


BEGIN_CRUSTA


template <typename PixelParam>
struct GlobeDataTraits
{
/*- fixed filtered lookups into two dimensional pixel domains. The lookups must
be aligned with the centers of the pixels in the domain. They are used for
subsampling. */
    enum SubsampleFilter
    {
        SUBSAMPLE_POINT,
        SUBSAMPLE_PYRAMID,
        SUBSAMPLE_LANCZOS5
    };

    ///type representing no data
    struct Nodata
    {
        Nodata();
        bool isNodata(const PixelParam& test) const;
        static Nodata parse(const std::string& nodataString);
    };

//- pixel operations
    ///average two values
    static PixelParam average(const PixelParam& a, const PixelParam& b);
    ///average four values
    static PixelParam average(const PixelParam& a, const PixelParam& b,
                              const PixelParam& c, const PixelParam& d);
    ///compute the minimum of two values
    static PixelParam minimum(const PixelParam& a, const PixelParam& b);
    ///compute the maximum of two values
    static PixelParam maximum(const PixelParam& a, const PixelParam& b);

//- arbitrary filtered lookups into two dimensional pixel domains
    ///nearest pixel lookup
    static PixelParam nearestLookup(const PixelParam* img,
        const int origin[2], const double at[2], const int size[2],
        const Nodata& nodata, const PixelParam& defaultValue);

    ///bilinear pixel lookup
    static PixelParam linearLookup(const PixelParam* img,
        const int origin[2], const double at[2], const int size[2],
        const Nodata& nodata, const PixelParam& defaultValue);

    ///width of the subsampling filter
    template <SubsampleFilter filter>
    static int subsampleWidth();

    ///subsampled lookup (requires knowledge on the row length of the domain)
    template <SubsampleFilter filter>
    static PixelParam subsample(PixelParam* at, int rowLen);

//- quadtree file management
    struct FileHeader;
    struct TileHeader;

    ///template type for a corresponding quadtree file
    typedef QuadtreeFile<PixelParam, FileHeader, TileHeader> File;

    ///generic header for file scope meta-data. Defaults to an empty header.
    struct FileHeader
    {
        void read(Misc::LargeFile* file);
        static size_t getSize();

#if CONSTRUO_BUILD
        void write(Misc::LargeFile* file) const;
#endif //CONSTRUO_BUILD
    };

    ///generic header for tile scope meta-data. Defaults to an empty header.
    struct TileHeader
    {
        void read(Misc::LargeFile* file);
        static size_t getSize();

#if CONSTRUO_BUILD
        template <typename NodeParam>
        TileHeader(NodeParam* node=NULL);
        template <typename NodeParam>
        void reset(NodeParam* node=NULL);

        void write(Misc::LargeFile* file) const;
#endif //CONSTRUO_BUILD
    };


//- database storage traits
    ///name for the type of the stored data (e.g. topography, imagery, etc.)
    static const char* typeName();
    ///number of channels of the stored data
    static const int numChannels();
};


END_CRUSTA


#include <crusta/GlobeColorTextureTraits.h>
#include <crusta/GlobeTopographyTraits.h>


#endif //_GlobeDataTraits_H_
