#ifndef _TextureColorGlobeData_H_
#define _TextureColorGlobeData_H_


#include <Math/Constants.h>

#include <crusta/GlobeData.h>
#include <crusta/TextureColor.h>

#if CONSTRUO_BUILD
#include <construo/Tree.h>
#endif //CONSTRUO_BUILD


BEGIN_CRUSTA


template <>
struct GlobeData<TextureColor>
{
    struct FileHeader;
    struct TileHeader;

    ///template type for a corresponding quadtree file
    typedef QuadtreeFile<TextureColor, FileHeader, TileHeader> File;

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
        TileHeader(TreeNode<TextureColor>* node=NULL)  {}
        void reset(TreeNode<TextureColor>* node=NULL)  {}

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

    static TextureColor defaultNodata()
    {
        return TextureColor(0,0,0);
    }
};


END_CRUSTA


#endif //_TextureColorGlobeData_H_
