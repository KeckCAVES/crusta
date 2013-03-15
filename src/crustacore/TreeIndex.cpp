#include <crustacore/TreeIndex.h>


#include <cassert>
#include <sstream>

#include <iostream>


namespace crusta {


const TreeIndex TreeIndex::invalid(~0,~0,~0,~0);


TreeIndex::
TreeIndex(uint8_t iPatch, uint8_t iChild, uint8_t iLevel, uint64_t iIndex)
{
    reserved(~0);
    patch(iPatch);
    child(iChild);
    level(iLevel);
    index(iIndex);
}
TreeIndex::
TreeIndex(const TreeIndex& i)
{
    reserved(~0);
    patch(i.patch());
    child(i.child());
    level(i.level());
    index(i.index());
}


TreeIndex& TreeIndex::
operator=(const TreeIndex& other)
{
    reserved(~0);
    patch(other.patch());
    child(other.child());
    level(other.level());
    index(other.index());
    return *this;
}

bool TreeIndex::
operator==(const TreeIndex& other) const
{
    return this->raw == other.raw;
}

bool TreeIndex::
operator!=(const TreeIndex& other) const
{
    return this->raw != other.raw;
}

uint8_t TreeIndex::
reserved() const
{
    return static_cast<uint8_t>(raw&0x1F);
}

void TreeIndex::
reserved(const uint8_t v)
{
    raw &= ~static_cast<uint64_t>(0x1F);
    raw |=  static_cast<uint64_t>(v&0x1F);
}

uint8_t TreeIndex::
patch() const
{
    return static_cast<uint8_t>((raw&0x3E0)>>5);
}

void TreeIndex::
patch(const uint8_t v)
{
    raw &= ~static_cast<uint64_t>(0x3E0);
    raw |=  static_cast<uint64_t>((v&0x1F)<<5);
}

uint8_t TreeIndex::
child() const
{
    return static_cast<uint8_t>((raw&0xC00)>>10);
}

void TreeIndex::
child(const uint8_t v)
{
    raw &= ~static_cast<uint64_t>(0xC00);
    raw |=  static_cast<uint64_t>((v&0x3)<<10);
}

uint8_t TreeIndex::
level() const
{
    return static_cast<uint8_t>((raw&0x3F000)>>12);
}

void TreeIndex::
level(const uint8_t v)
{
    raw &= ~static_cast<uint64_t>(0x3F000);
    raw |=  static_cast<uint64_t>((v&0x3F)<<12);
}

uint64_t TreeIndex::
index() const
{
    return (raw&0xFFFFFFFFFFFC0000)>>18;
}

void TreeIndex::
index(const uint64_t v)
{
    raw &= ~0xFFFFFFFFFFFC0000;
    raw |=  (v&0x3FFFFFFFFFFF)<<18;
}




TreeIndex TreeIndex::
up() const
{
    assert(level()>0);
    if (level() <= 1)
        return TreeIndex(patch(), 0, 0, 0);
    else
    {
        uint8_t  newChild = static_cast<uint8_t>(((index())>>((level()-2)*2))&0x3);
        uint64_t newIndex = index();
        newIndex = newIndex & ~(((uint64_t)(0x3)) << ((level()-1) * 2));
        return TreeIndex(patch(), newChild, level()-1, newIndex);
    }
}

TreeIndex TreeIndex::
down(uint8_t which) const
{
    uint64_t newIndex = index() | (static_cast<uint64_t>(which) << (level()*2));
    return TreeIndex(patch(), which, level()+1, newIndex);
}


std::string TreeIndex::
str() const
{
    std::ostringstream os;

    if (level() == invalid.level())
        return "i";
    if (level() == 0)
        return "r";

    uint64_t i = index();
    for (size_t j=0; j<level(); ++j, i>>=2)
        os << (i&0x3);

    return os.str();
}

std::string TreeIndex::
med_str() const
{
    std::ostringstream os;
    if (reserved() == invalid.reserved())
        os << "i|";
    else
        os << static_cast<int>(reserved()) << "|";
    if (patch() == invalid.patch())
        os << "i.";
    else
        os << static_cast<int>(patch()) << ".";

    if (level() == invalid.level())
    {
        os << "i";
        return os.str();
    }
    if (level() == 0)
    {
        os << "r";
        return os.str();
    }

    uint64_t i = index();
    for (size_t j=0; j<level(); ++j, i>>=2)
        os << (i&0x3);

    return os.str();
}

std::ostream&
operator<<(std::ostream& os, const TreeIndex& ti)
{
    if (ti.level() == TreeIndex::invalid.level())
        return os << std::string("i");
    if (ti.level() == 0)
        return os << std::string("r");

    uint64_t i = ti.index();
    for (size_t j=0; j<ti.level(); ++j, i>>=2)
        os << (i&0x3);

    return os;
}




TreePath::
TreePath(TreeIndex i) :
    level(i.level()==0?0:i.level()+1), index(i.index())
{}

uint8_t TreePath::
pop()
{
    if (level==0)
        return END;

    uint8_t retVal = index & 0x3;
    index >>= 2;

    return retVal;
}

} //namespace crusta
