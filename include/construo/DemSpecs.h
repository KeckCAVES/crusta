///\todo GPL

#ifndef _DemSpecs_H_
#define _DemSpecs_H_

#include <construo/QuadtreeFileHeaders.h>
#include <construo/Tree.h>

BEGIN_CRUSTA

/// data type for values of a DEM stored in the preprocessed hierarchies
typedef float DemHeight;

template <>
class TreeState<DemHeight>
{
public:
    typedef QuadtreeFile< DemHeight, QuadtreeFileHeader<DemHeight>,
                          QuadtreeTileHeader<DemHeight>             > File;

    File* file;
};

template <>
struct QuadtreeTileHeader<DemHeight> : public QuadtreeTileHeaderBase<DemHeight>
{
public:
    QuadtreeTileHeader()
    {
        reset();
    }
    QuadtreeTileHeader(TreeNode<DemHeight>* node)
    {
        reset(node);
    }

	static size_t getSize()
    {
		return 2 * sizeof(DemHeight);
    }
    
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
    
	void read(Misc::LargeFile* file)
    {
		file->read(range, 2);
    }
	void write(Misc::LargeFile* file) const
    {
		file->write(range, 2);
    }

    static const DemHeight defaultPixelValue;

    ///range of height values of DEM tile
	DemHeight range[2];
};


END_CRUSTA

#endif //_DemSpecs_H_
