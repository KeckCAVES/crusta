#ifndef _DemHeightGlobeData_H_
#define _DemHeightGlobeData_H_


#include <crusta/DemHeight.h>
#include <crusta/GlobeData.h>

#include <sstream>

#include <Math/Constants.h>


BEGIN_CRUSTA


template <>
struct GlobeData<DemHeight>
{
    struct FileHeader;
    struct TileHeader;

    ///template type for a corresponding quadtree file
    typedef QuadtreeFile<DemHeight, FileHeader, TileHeader> File;

    struct FileHeader
    {
    public:
        void read(Misc::LargeFile* file)         {}
        static size_t getSize()                  {return 0;}

#if CONSTRUO_BUILD
        void write(Misc::LargeFile* file) const  {}
#endif //CONSTRUO_BUILD
    };

    struct TileHeader
    {
        ///range of height values of DEM tile
        DemHeight range[2];

        void read(Misc::LargeFile* file)
        {
            file->read(range, 2);
        }

        static size_t getSize()
        {
            return 2 * sizeof(DemHeight);
        }

#if CONSTRUO_BUILD
        TileHeader()
        {
            range[0] =  Math::Constants<DemHeight>::max;
            range[1] = -Math::Constants<DemHeight>::max;
        }

        void write(Misc::LargeFile* file) const
        {
            file->write(range, 2);
        }
#endif //CONSTRUO_BUILD
    };

    static const std::string typeName()
    {
        return "Topography";
    }

    static const int numChannels()
    {
        return 1;
    }

    static std::string defaultPolyhedronType()
    {
        return "Triacontahedron";
    }

    static DemHeight defaultNodata()
    {
        return DemHeight(-4.294967296e+9);
    }
};


END_CRUSTA


#endif //_DemHeightGlobeData_H_
