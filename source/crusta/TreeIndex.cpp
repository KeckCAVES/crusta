#include <crusta/TreeIndex.h>

#include <cassert>
#include <sstream>

BEGIN_CRUSTA

const TreeIndex TreeIndex::invalid(~0,~0,~0,~0);

size_t TreeIndex::hash::
operator() (const TreeIndex& i) const
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


TreeIndex::
TreeIndex(uint8 iPatch, uint8 iChild, uint8 iLevel, uint64 iIndex) :
    reserved(0), patch(iPatch), child(iChild), level(iLevel), index(iIndex)
{}
TreeIndex::
TreeIndex(const TreeIndex& i) :
    reserved(0), patch(i.patch), child(i.child), level(i.level), index(i.index)
{}


TreeIndex TreeIndex::
up() const
{
    assert(level>0);
    if (level <= 1)
        return TreeIndex(patch, 0, 0, 0);
    else
    {
        uint8  newChild = (((uint64)(index)) >> ((level-2) * 2)) & 0x3;
        uint64 newIndex = ((uint64)(index));
        newIndex = newIndex & ~(((uint64)(0x3)) << ((level-1) * 2));
        return TreeIndex(patch, newChild, level-1, newIndex);
    }
}

TreeIndex TreeIndex::
down(uint8 which) const
{
    uint64 newIndex = ((uint64)(index)) | (((uint64)(which)) << (level*2));
    return TreeIndex(patch, which, level+1, newIndex);
}

bool TreeIndex::
operator==(const TreeIndex& other) const
{
    return *(reinterpret_cast<const uint64*>(this)) ==
           *(reinterpret_cast<const uint64*>(&other));
}


std::string TreeIndex::
str() const
{
    std::ostringstream os;

    if (level == uint8(~0))
        return "i";
    if (level == 0)
        return "r";

    uint64 i = index;
    for (uint j=0; j<level; ++j, i>>=2)
        os << (i&0x3);

    return os.str();
}

std::string TreeIndex::
med_str() const
{
    std::ostringstream os;
    os << patch << ".";

    if (level == uint8(~0))
        return "i";
    if (level == 0)
    {
        os << "r";
        return os.str();
    }

    uint64 i = index;
    for (uint j=0; j<level; ++j, i>>=2)
        os << (i&0x3);

    return os.str();
}

std::ostream&
operator<<(std::ostream& os, const TreeIndex& i)
{
    if (i.level == uint8(~0))
        return os << std::string("i");
    if (i.level == 0)
        return os << std::string("r");

    uint64 index = i.index;
    for (uint j=0; j<i.level; ++j, index>>=2)
        os << (index&0x3);

    return os;
}




TreePath::
TreePath(TreeIndex i) :
    level(i.level==0?0:i.level+1), index(i.index)
{}

uint8 TreePath::
pop()
{
    if (level==0)
        return END;

    uint8 retVal = index & 0x3;
    index >>= 2;

    return retVal;
}

END_CRUSTA
