#include <crusta/DataIndex.h>


BEGIN_CRUSTA


const DataIndex DataIndex::
invalid(TreeIndex::invalid.reserved(), TreeIndex::invalid);

size_t DataIndex::hash::
operator() (const DataIndex& i) const
{
    //taken from http://www.concentric.net/~Ttwang/tech/inthash.htm

    uint64 key = i.raw;
    key = (~key) + (key << 18); // key = (key << 18) - key - 1;
    key = key ^ (key >> 31);
    key = key * 21; // key = (key + (key << 2)) + (key << 4);
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);

    return static_cast<size_t>(static_cast<uint32>(key));
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
    reserved(dataId);
}

DataIndex::
DataIndex(const DataIndex& i) :
    TreeIndex(i.patch(), i.child(), i.level(), i.index())
{
    reserved(i.reserved());
}

DataIndex& DataIndex::
operator=(const DataIndex& other)
{
    reserved(other.reserved());
    patch(other.patch());
    child(other.child());
    level(other.level());
    index(other.index());
    return *this;
}

bool DataIndex::
operator==(const DataIndex& other) const
{
    return raw==other.raw;
}


uint8 DataIndex::
getDataId() const
{
    return reserved();
}

TreeIndex DataIndex::
getTreeIndex() const
{
    return TreeIndex(patch(), child(), level(), index());
}


END_CRUSTA
