#ifndef _GlobeData_H_
#define _GlobeData_H_


#include <string>

#include <crusta/QuadtreeFile.h>


BEGIN_CRUSTA


template <typename PixelParam>
struct GlobeData
{
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
        void write(Misc::LargeFile* file) const;
#endif //CONSTRUO_BUILD
    };


//- database storage traits
    ///name for the type of the stored data (e.g. topography, imagery, etc.)
    static const std::string typeName();
    ///number of channels of the stored data
    static const int numChannels();

///\todo these should go into a construo configuration file
    static std::string defaultPolyhedronType();
    static PixelParam  defaultNodata();
};


END_CRUSTA


#include <crusta/DemHeightGlobeData.h>
#include <crusta/TextureColorGlobeData.h>


#endif //_GlobeData_H_
