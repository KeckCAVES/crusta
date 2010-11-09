///\todo GPL

#ifndef _DemSpecs_H_
#define _DemSpecs_H_

#include <cassert>

#include <Math/Constants.h>
#include <Misc/LargeFile.h>

#include <crusta/PixelSpecs.h>
#include <crusta/QuadtreeFileHeaders.h>

#if CONSTRUO_BUILD
#include <construo/DynamicFilter.h>
#include <construo/Filter.h>
#include <construo/Tree.h>
#endif //CONSTRUO_BUILD

BEGIN_CRUSTA

/// data type for values of a DEM stored in the preprocessed hierarchies
typedef float DemHeight;

inline DemHeight
pixelAvg(const DemHeight& a, const DemHeight& b)
{
    return (a+b) * DemHeight(0.5);
}
inline DemHeight
pixelAvg(const DemHeight& a, const DemHeight& b, const DemHeight& c,
         const DemHeight& d)
{
    return (a+b+c+d) * DemHeight(0.25);
}

inline DemHeight
pixelMin(const DemHeight& one, const DemHeight& two)
{
    return one<two ? one : two;
}
inline DemHeight
pixelMax(const DemHeight& one, const DemHeight& two)
{
    return one>two ? one : two;
}

template <>
struct Nodata<DemHeight>
{
    Nodata() : value(Math::Constants<double>::max)
    {
    }

    double value;
    bool isNodata(const DemHeight& test) const
    {
        return double(test) == value;
    }
};

#if CONSTRUO_BUILD
namespace filter {

template <>
inline DemHeight
nearestLookup(const DemHeight* img, const int origin[2],
              const Scalar at[2], const int size[2],
              const Nodata<DemHeight>& nodata,
              const DemHeight& defaultValue)
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

template <>
inline DemHeight
linearLookup(const DemHeight* img, const int origin[2],
              const Scalar at[2], const int size[2],
              const Nodata<DemHeight>& nodata,
              const DemHeight& defaultValue)
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

} //end namespace filter

template <>
inline
DemHeight Filter::
lookup(DemHeight* at, uint rowLen)
{
    Scalar res(0);

    for (int y=-width; y<=static_cast<int>(width); ++y)
    {
        DemHeight* atY = at + y*rowLen;
        for (int x=-width; x<=static_cast<int>(width); ++x)
        {
            DemHeight* atYX = atY + x;
            res += Scalar(*atYX) * weights[y]*weights[x];
        }
    }

    return (DemHeight)res;
}

template <>
class TreeState<DemHeight>
{
public:
    typedef QuadtreeFile< DemHeight, QuadtreeFileHeader<DemHeight>,
                          QuadtreeTileHeader<DemHeight>             > File;

    File* file;
};

template <>
const char*
globeDataType<DemHeight>()
{
    return "Topography";
}
#endif //CONSTRUO_BUILD

template <>
struct QuadtreeTileHeader<DemHeight> : public QuadtreeTileHeaderBase<DemHeight>
{
public:
    QuadtreeTileHeader()
    {
        range[0] = range[1] = defaultPixelValue;
    }
#if CONSTRUO_BUILD
    QuadtreeTileHeader(TreeNode<DemHeight>* node)
    {
        reset(node);
    }
#endif //CONSTRUO_BUILD

    static size_t getSize()
    {
        return 2 * sizeof(DemHeight);
    }

#if CONSTRUO_BUILD
    void reset(TreeNode<DemHeight>* node=NULL)
    {
        if (node==NULL || node->data==NULL)
        {
            range[0] = range[1] = defaultPixelValue;
        }
        else
        {
            //calculate the tile's pixel value range
            DemHeight* tile = node->data;
            range[0] = range[1] = tile[0];

///\todo OpenMP this
            assert(node->treeState!=NULL && node->treeState->file!=NULL);
            const uint32* tileSize = node->treeState->file->getTileSize();
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
                    assert(child.tileIndex != TreeState<DemHeight>::
                           File::INVALID_TILEINDEX);
                    //get the child header
                    QuadtreeTileHeader<DemHeight> header;
#if DEBUG
                    bool res = node->treeState->file->readTile(
                        child.tileIndex, header);
                    assert(res==true);
#else
                    node->treeState->file->readTile(child.tileIndex, header);
#endif //DEBUG

                    range[0] = std::min(range[0], header.range[0]);
                    range[1] = std::max(range[1], header.range[1]);
                }
            }
        }
    }
#endif //CONSTRUO_BUILD

    void read(Misc::LargeFile* file)
    {
        file->read(range, 2);
    }
#if CONSTRUO_BUILD
    void write(Misc::LargeFile* file) const
    {
        file->write(range, 2);
    }
#endif //CONSTRUO_BUILD

    static const DemHeight defaultPixelValue;

    ///range of height values of DEM tile
    DemHeight range[2];
};


END_CRUSTA

#endif //_DemSpecs_H_
