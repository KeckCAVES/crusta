#ifndef _DemHeightGlobeData_H_
#define _DemHeightGlobeData_H_


#include <crusta/GlobeData.h>

#include <Math/Constants.h>

#include <crusta/DemHeight.h>


BEGIN_CRUSTA


template <>
struct GlobeData<DemHeight>
{
    typedef DemHeight::Type PixelType;

    struct FileHeader;
    struct TileHeader;

    ///template type for a corresponding quadtree file
    typedef QuadtreeFile<PixelType, FileHeader, TileHeader> File;

    struct FileHeader
    {
    public:
        void read(Misc::LargeFile* file)         {}
        static size_t getSize()                  {return 0;}
        void write(Misc::LargeFile* file) const  {}
    };

    struct TileHeader
    {
        ///range of height values of DEM tile
        PixelType range[2];

        void read(Misc::LargeFile* file)
        {
            file->read(range, 2);
        }

        static size_t getSize()
        {
            return 2 * sizeof(PixelType);
        }

        TileHeader()
        {
            range[0] =  Math::Constants<PixelType>::max;
            range[1] = -Math::Constants<PixelType>::max;
        }

        void write(Misc::LargeFile* file) const
        {
            file->write(range, 2);
        }
    };

    static const std::string typeName()
    {
        return std::string("Topography");
    }

    static const int numChannels()
    {
        return 1;
    }

    static std::string defaultPolyhedronType()
    {
        return std::string("Triacontahedron");
    }

    static PixelType defaultNodata()
    {
        return PixelType(-4.294967296e+9);
    }
};


END_CRUSTA


#endif //_DemHeightGlobeData_H_
