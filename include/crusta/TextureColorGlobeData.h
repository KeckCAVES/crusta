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

#if CONSTRUO_BUILD
        void write(Misc::LargeFile*) const {}
#endif //CONSTRUO_BUILD
    };

    struct TileHeader
    {
        void read(Misc::LargeFile*)       {}
        static size_t getSize()           {return 0;}

#if CONSTRUO_BUILD
        void write(Misc::LargeFile*) const{}
#endif //CONSTRUO_BUILD
    };

    static const std::string typeName()
    {
        return "ImageRGB";
    }

    static const int numChannels()
    {
        return 3;
    }

    static std::string defaultPolyhedronType()
    {
        return "Triacontahedron";
    }

    static PixelType defaultNodata()
    {
        return PixelType(0,0,0);
    }
};


END_CRUSTA


#endif //_TextureColorGlobeData_H_
