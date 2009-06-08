///\todo GPL

#ifndef _DemSpecs_H_
#define _DemSpecs_H_

#include <cassert>

#include <Misc/LargeFile.h>

#include <crusta/PixelSpecs.h>
#include <crusta/QuadtreeFileHeaders.h>

#if CONSTRUO_BUILD
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
    double value;
    bool isNodata(const DemHeight& test) const
    {
        return double(test) == value;
    }
};

#if CONSTRUO_BUILD
template <>
class TreeState<DemHeight>
{
public:
    typedef QuadtreeFile< DemHeight, QuadtreeFileHeader<DemHeight>,
                          QuadtreeTileHeader<DemHeight>             > File;

    File* file;
};
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
            const uint* tileSize = node->treeState->file->getTileSize();
            for(uint i=1; i<tileSize[0]*tileSize[1]; ++i)
			{
                range[0] = std::min(range[0], tile[i]);
                range[1] = std::max(range[1], tile[i]);
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
