#ifndef _GlobeTopographyTraits_H_
#define _GlobeTopographyTraits_H_


#include <crusta/GlobeDataTraits.h>

#include <sstream>

#include <Math/Constants.h>

#if CONSTRUO_BUILD
#include <construo/Tree.h>
#endif //CONSTRUO_BUILD


BEGIN_CRUSTA


/// data type for values of a DEM stored in the preprocessed hierarchies
typedef float DemHeight;


template <>
struct GlobeDataTraits<DemHeight>
{
    typedef double Scalar;

/*- fixed filtered lookups into two dimensional pixel domains. The lookups must
be aligned with the centers of the pixels in the domain. They are used for
subsampling. */
    enum SubsampleFilter
    {
        SUBSAMPLE_POINT,
        SUBSAMPLE_PYRAMID,
        SUBSAMPLE_LANCZOS5
    };

    struct Nodata
    {
        Nodata() :
            value(Math::Constants<double>::max)
        {
        }

        bool isNodata(const DemHeight& test) const
        {
            return double(test) == value;
        }

        static Nodata parse(const std::string& nodataString)
        {
            Nodata nodata;
            std::istringstream iss(nodataString);
            iss >> nodata.value;
            return nodata;
        }

        double value;
    };


    static DemHeight average(const DemHeight& a, const DemHeight& b)
    {
        return (a+b) * DemHeight(0.5);
    }
    static DemHeight average(const DemHeight& a, const DemHeight& b,
                              const DemHeight& c, const DemHeight& d)
    {
        return (a+b+c+d) * DemHeight(0.25);
    }
    static DemHeight minimum(const DemHeight& a, const DemHeight& b)
    {
        return std::min(a, b);
    }
    static DemHeight maximum(const DemHeight& a, const DemHeight& b)
    {
        return std::max(a, b);
    }


    static DemHeight nearestLookup(
        const DemHeight* img, const int origin[2],
        const Scalar at[2], const int size[2],
        const Nodata& nodata, const DemHeight& defaultValue)
    {
        int ip[2];
        for (uint i=0; i<2; ++i)
        {
            Scalar pFloor = Math::floor(at[i] + Scalar(0.5));
            ip[i]         = static_cast<int>(pFloor) - origin[i];
        }
        DemHeight imgValue = img[ip[1]*size[0] + ip[0]];
        return nodata.isNodata(imgValue) ? defaultValue : imgValue;
    }

    static DemHeight linearLookup(
        const DemHeight* img, const int origin[2],
        const Scalar at[2], const int size[2],
        const Nodata& nodata, const DemHeight& defaultValue)
    {
        int ip[2];
        Scalar d[2];
        for(uint i=0; i<2; ++i)
        {
            Scalar pFloor = Math::floor(at[i]);
            d[i]          = at[i] - pFloor;
            ip[i]         = static_cast<int>(pFloor) - origin[i];
            if (ip[i] == size[i]-1)
            {
                --ip[i];
                d[i] = 1.0;
            }
        }
        const DemHeight* base = img + (ip[1]*size[0] + ip[0]);
        Scalar v[4];
        v[0] = nodata.isNodata(base[0])         ? defaultValue : base[0];
        v[1] = nodata.isNodata(base[1])         ? defaultValue : base[1];
        v[2] = nodata.isNodata(base[size[0]])   ? defaultValue : base[size[0]];
        v[3] = nodata.isNodata(base[size[0]+1]) ? defaultValue : base[size[0]+1];

        Scalar one  = (1.0-d[0])*v[0] + d[0]*v[1];
        Scalar two  = (1.0-d[0])*v[2] + d[0]*v[3];
        return DemHeight((1.0-d[1])*one  + d[1]*two);
    }


    template <SubsampleFilter filter>
    static int subsampleWidth();

    template <>
    static int subsampleWidth<SUBSAMPLE_POINT>()
    {
        return 0;
    }
    template <>
    static int subsampleWidth<SUBSAMPLE_PYRAMID>()
    {
        return 1;
    }
    template <>
    static int subsampleWidth<SUBSAMPLE_LANCZOS5>()
    {
        return 10;
    }

    template <SubsampleFilter filter>
    static void subsample(PixelParam* at, int rowLen);

    template <>
    static DemHeight subsample<SUBSAMPLE_POINT>(PixelParam* at, int rowLen)
    {
        return *at;
    }
    template <>
    static DemHeight subsample<SUBSAMPLE_PYRAMID>(PixelParam* at, int rowLen)
    {
        static double weightStorage[3] = {0.25, 0.50, 0.25};
        double* weights = &weightStorage[1];

        double res(0);

        for (int y=-1; y<=1; ++y)
        {
            DemHeight* atY = at + y*rowLen;
            for (int x=-1; x<=1; ++x)
            {
                DemHeight* atYX = atY + x;
                res += Scalar(*atYX) * weights[y]*weights[x];
            }
        }

        return (DemHeight)res;
    }
    template <>
    static DemHeight subsample<SUBSAMPLE_LANCZOS5>(PixelParam* at, int rowLen)
    {
        static const double weightStorage[21] = {
             7.60213661720011e-34,  0.00386785330198227,  -4.5610817871754e-18,
              -0.0167391813072476, 9.83998047615722e-18,    0.0405539013275657,
            -1.47599707142358e-17,  -0.0911355426727928,  1.82443271487016e-17,
                0.313296117460564,    0.500313703779858,     0.313296117460564,
             1.82443271487016e-17,  -0.0911355426727928, -1.47599707142358e-17,
               0.0405539013275657, 9.83998047615722e-18,   -0.0167391813072476,
             -4.5610817871754e-18,  0.00386785330198227,  7.60213661720011e-34};
        double* weights = &weightStorage[10];

        double res(0);

        for (int y=-10; y<=10; ++y)
        {
            DemHeight* atY = at + y*rowLen;
            for (int x=-10; x<=10; ++x)
            {
                DemHeight* atYX = atY + x;
                res += Scalar(*atYX) * weights[y]*weights[x];
            }
        }

        return (DemHeight)res;
    }


    struct FileHeader;
    struct TileHeader;

    ///template type for a corresponding quadtree file
    typedef QuadtreeFile<PixelParam, FileHeader, TileHeader> File;

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
        template <>
        TileHeader(TreeNode<DemHeight>* node=NULL)
        {
            reset(node);
        }

        template <>
        void reset(TreeNode<DemHeight>* node=NULL)
        {
            if (node==NULL || node->data==NULL)
            {
                range[0] = range[1] = 0.0f;
                return;
            }

            //calculate the tile's pixel value range
            DemHeight* tile = node->data;
            range[0] = range[1] = tile[0];

///\todo OpenMP this
            assert(node->globeFile != NULL);
            File& file = node->globeFile->getPatch(node->treeIndex.patch);
            const uint32* tileSize = file.getTileSize();
            for(uint i=1; i<tileSize[0]*tileSize[1]; ++i)
            {
                range[0] = std::min(range[0], tile[i]);
                range[1] = std::max(range[1], tile[i]);
            }

            if (node->children != NULL)
            {
                for (int i=0; i<4; ++i)
                {
                    TreeNode<DemHeight>& child = node->children[i];
                    assert(child.tileIndex != File::INVALID_TILEINDEX);
                    //get the child header
                    TileHeader header;
#if DEBUG
                    bool res = file.readTile(child.tileIndex, header);
                    assert(res==true);
#else
                    file.readTile(child.tileIndex, header);
#endif //DEBUG

                    range[0] = std::min(range[0], header.range[0]);
                    range[1] = std::max(range[1], header.range[1]);
                }
            }
        }

        void write(Misc::LargeFile* file) const
        {
            file->write(range, 2);
        }
#endif //CONSTRUO_BUILD
    };

    static const char* typeName()
    {
        return "Topography";
    }

    static const int numChannels()
    {
        return 1;
    }
};


END_CRUSTA


#endif //_GlobeTopographyTraits_H_
