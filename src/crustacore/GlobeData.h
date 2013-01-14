#ifndef _GlobeData_H_
#define _GlobeData_H_


#include <string>

#include <crustacore/QuadtreeFile.h>


BEGIN_CRUSTA


template <typename PixelParam>
struct GlobeData
{
    typedef typename PixelParam::Type PixelType;

//- quadtree file management
    struct FileHeader;
    struct TileHeader;

    ///template type for a corresponding quadtree file
    typedef QuadtreeFile<PixelType, FileHeader, TileHeader> File;

    ///generic header for file scope meta-data. Defaults to an empty header.
    struct FileHeader
    {
        void read(Misc::LargeFile* file);
        static size_t getSize();
        void write(Misc::LargeFile* file) const;
    };

    ///generic header for tile scope meta-data. Defaults to an empty header.
    struct TileHeader
    {
        void read(Misc::LargeFile* file);
        static size_t getSize();
        void write(Misc::LargeFile* file) const;
    };

//- database storage traits
    ///name for the type of the stored data (e.g. topography, imagery, etc.)
    static const std::string typeName();
    ///number of channels of the stored data
    static const int numChannels();

///\todo these should go into a construo configuration file
    static std::string defaultPolyhedronType();
    static PixelType  defaultNodata();
};


END_CRUSTA


#include <crustacore/DemHeightGlobeData.h>
#include <crustacore/LayerDataGlobeData.h>
#include <crustacore/TextureColorGlobeData.h>


#endif //_GlobeData_H_
