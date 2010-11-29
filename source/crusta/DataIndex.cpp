#include <crusta/DataIndex.h>


BEGIN_CRUSTA


const DataIndex DataIndex::invalid(~0, TreeIndex::invalid);

size_t DataIndex::hash::
operator() (const DataIndex& i) const
{
    //taken from http://www.concentric.net/~Ttwang/tech/inthash.htm

    uint64 key = *(reinterpret_cast<const uint64*>(&i));
    key = (~key) + (key << 18); // key = (key << 18) - key - 1;
    key = key ^ (key >> 31);
    key = key * 21; // key = (key + (key << 2)) + (key << 4);
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);

    return static_cast<size_t>(key);
}

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

DataIndex::
DataIndex(const DataIndex& i) :
    TreeIndex(i.patch, i.child, i.level, i.index)
{
    reserved = i.reserved;
}

DataIndex& DataIndex::
operator=(const DataIndex& other)
{
    reserved = other.reserved;
    patch    = other.patch;
    child    = other.child;
    level    = other.level;
    index    = other.index;
    return *this;
}

bool DataIndex::
operator==(const DataIndex& other) const
{
    return *(reinterpret_cast<const uint64*>(this)) ==
           *(reinterpret_cast<const uint64*>(&other));
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
