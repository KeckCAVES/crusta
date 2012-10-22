#ifndef _TextureColorGlobeData_H_
#define _TextureColorGlobeData_H_


#include <crusta/GlobeData.h>

#include <Math/Constants.h>

#include <crusta/TextureColor.h>


BEGIN_CRUSTA


template <>
struct GlobeData<TextureColor>
{
    typedef TextureColor::Type PixelType;

    struct FileHeader;
    struct TileHeader;

    ///template type for a corresponding quadtree file
    typedef QuadtreeFile<PixelType, FileHeader, TileHeader> File;

    struct FileHeader
    {
    public:
        void read(Misc::LargeFile*)        {}
        static size_t getSize()            {return 0;}
        void write(Misc::LargeFile*) const {}
    };

    struct TileHeader
    {
        void read(Misc::LargeFile*)        {}
        static size_t getSize()            {return 0;}
        void write(Misc::LargeFile*) const {}
    };

    static const std::string typeName()
    {
        return std::string("ImageRGB");
    }

    static const int numChannels()
    {
        return 3;
    }

    static std::string defaultPolyhedronType()
    {
        return std::string("Triacontahedron");
    }

    static PixelType defaultNodata()
    {
        return PixelType(0,0,0);
    }
};


END_CRUSTA


#endif //_TextureColorGlobeData_H_
