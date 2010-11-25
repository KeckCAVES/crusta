#ifndef _DataIndex_H_
#define _DataIndex_H_


#include <crusta/TreeIndex.h>


BEGIN_CRUSTA

/** uniquely specifies a data element for use in caching */
struct DataIndex : public TreeIndex
{
    DataIndex();
    DataIndex(uint8 dataId, const TreeIndex& treeIndex);

    uint8 getDataId() const;
    TreeIndex getTreeIndex() const;

    static const DataIndex invalid;
};


END_CRUSTA


#endif //_DataIndex_H_
