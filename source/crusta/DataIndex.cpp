#include <crusta/DataIndex.h>


BEGIN_CRUSTA


const DataIndex DataIndex::invalid(~0, TreeIndex::invalid);

DataIndex::
DataIndex()
{
    *this = invalid;
}

DataIndex::
DataIndex(uint8 dataId, const TreeIndex& treeIndex) :
    TreeIndex(treeIndex)
{
    reserved = dataId;
}


uint8 DataIndex::
getDataId() const
{
    return reserved;
}

TreeIndex DataIndex::
getTreeIndex() const
{
    return TreeIndex(patch, child, level, index);
}


END_CRUSTA
