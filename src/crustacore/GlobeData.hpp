#ifndef _GlobeData_H_
#define _GlobeData_H_


#include <crustacore/GlobeData.h>

#include <string>

#include <Math/Constants.h>


namespace crusta {


//- Topography -----------------------------------------------------------------

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
            return 2 * sizeof(DemHeight);
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
    
    static PixelType defaultNodata()
    {
        return PixelType(-4.294967296e+9);
    }
};


//- TextureColor ---------------------------------------------------------------

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
        void read(Misc::LargeFile*)       {}
        static size_t getSize()           {return 0;}
        void write(Misc::LargeFile*) const{}
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


} //namespace crusta


#endif //_GlobeData_H_
