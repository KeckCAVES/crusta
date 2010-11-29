#ifndef _TreeIndex_H_
#define _TreeIndex_H_

#include <string>

#include <crusta/basics.h>

BEGIN_CRUSTA

/** uniquely specifies a node in the quadtree hierarchy */
struct TreeIndex
{
    TreeIndex(uint8 iPatch=0,uint8 iChild=0,uint8 iLevel=0,uint64 iIndex=0);
    TreeIndex(const TreeIndex& i);

    TreeIndex& operator=(const TreeIndex& other);
    bool operator==(const TreeIndex& other) const;
    
    TreeIndex up() const;
    TreeIndex down(uint8 which) const;

    std::string str() const;
    std::string med_str() const;
    friend std::ostream& operator<<(std::ostream& os, const TreeIndex& i);

    static const TreeIndex invalid;

    uint64 reserved : 5; ///< reserved for differentiating same indices
    uint64 patch    : 5; ///< index of the base patch of the global hierarchy
    uint64 child    : 2; ///< index within the group of siblings
    uint64 level    : 6; ///< level in the global hierarchy (0 is root)
    /** describes a path from the root to the indicated node as a sequence
     of two-bit child-indices. The sequence starts with the least
     significant bits. */
    uint64 index    : 46;
};

/** A traversal helper for following the path from the root to a node specified
    by the tree index provided during construction. Only a forward traversal is
    supported */
struct TreePath
{
    static const uint8 END = ~0x0;

    TreePath(TreeIndex i);

    /** returns the index of the child to go to for continueing the traversal.
     The end is reached when the returned child index is END */
    uint8 pop();

    uint8 level;
    uint64 index;
};


END_CRUSTA

#endif //_TreeIndex_H_
