#ifndef _LayerDataGlobeData_H_
#define _LayerDataGlobeData_H_


#include <crusta/GlobeData.h>

#include <Math/Constants.h>

#include <crusta/LayerData.h>


BEGIN_CRUSTA


template <typename TypeParam>
struct GlobeData< LayerData<TypeParam> >
{
    typedef LayerData<TypeParam>      LayerDataT;
    typedef typename LayerDataT::Type PixelType;

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
        void read(Misc::LargeFile*)        {}
        static size_t getSize()            {return 0;}
        
#if CONSTRUO_BUILD
        void write(Misc::LargeFile*) const {}
#endif //CONSTRUO_BUILD
    };

    static const std::string typeName()
    {
        return std::string("LayerData") + LayerDataT::suffix();
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
        return LayerDataT::defaultNodata();
    }
};


END_CRUSTA


#endif //_LayerDataGlobeData_H_
