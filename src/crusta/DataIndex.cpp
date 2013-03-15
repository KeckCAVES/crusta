#include <crusta/DataIndex.h>


namespace crusta {


const DataIndex DataIndex::
invalid(TreeIndex::invalid.reserved(), TreeIndex::invalid);

size_t DataIndex::hash::
operator() (const DataIndex& i) const
{
    //taken from http://www.concentric.net/~Ttwang/tech/inthash.htm

    uint64_t key = i.raw;
    key = (~key) + (key << 18); // key = (key << 18) - key - 1;
    key = key ^ (key >> 31);
    key = key * 21; // key = (key + (key << 2)) + (key << 4);
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);

    return static_cast<size_t>(static_cast<uint32_t>(key));
}

DataIndex::
DataIndex()
{
    *this = invalid;
}

DataIndex::
DataIndex(uint8_t dataId, const TreeIndex& treeIndex) :
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


uint8_t DataIndex::
getDataId() const
{
    return reserved();
}

TreeIndex DataIndex::
getTreeIndex() const
{
    return TreeIndex(patch(), child(), level(), index());
}


} //namespace crusta
