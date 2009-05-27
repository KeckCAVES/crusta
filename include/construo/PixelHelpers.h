#ifndef _PixelHelpers_H_
#define _PixelHelpers_H_

#include <crusta/DemSpecs.h>

#define BEGIN_PIXEL namespace pixel {
#define END_PIXEL } //end namespace pixel

BEGIN_CRUSTA

BEGIN_PIXEL


template <typename PixelParam>
inline void
relativeToAverageElevation(TreeNode<PixelParam>* node,
                           const QuadtreeTileHeader<PixelParam>& header)
{}

template <>
inline void
relativeToAverageElevation<DemHeight>(
    TreeNode<DemHeight>* node, const QuadtreeTileHeader<DemHeight>& header)
{
    DemHeight average = (header.range[0] + header.range[1]) * DemHeight(0.5);
    
    const uint* tileSize = node->treeState->file->getTileSize();
    for (DemHeight* h=node->data; h<node->data+tileSize[0]*tileSize[1]; ++h)
        *h -= average;
}


END_PIXEL

END_CRUSTA

#endif //_PixelHelpers_H_
