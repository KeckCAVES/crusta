#ifndef _TreeIndex_H_
#define _TreeIndex_H_

#include <string>

#include <crusta/basics.h>

BEGIN_CRUSTA

/** uniquely specifies a node in the quadtree hierarchy */
struct TreeIndex
{
    struct hash {
        size_t operator() (const TreeIndex& i) const;
    };
    
    TreeIndex(uint8 iPatch=0,uint8 iChild=0,uint16 iLevel=0,uint32 iIndex=0);
    TreeIndex(const TreeIndex& i);
    
    TreeIndex up() const;
    TreeIndex down(uint8 which) const;

    bool operator==(const TreeIndex& other) const;

    std::string str() const;
    std::string med_str() const;
    friend std::ostream& operator<<(std::ostream& os, const TreeIndex& i);
    
    uint64 patch :  8; ///< index of the base patch of the global hierarchy
    uint64 child :  8; ///< index within the group of siblings
    uint64 level : 16; ///< level in the global hierarchy (0 is root)
    /** describes a path from the root to the indicated node as a sequence
     of two-bit child-indices. The sequence starts with the least
     significant bits. */
    uint64 index : 32;
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
    
    uint16 level;
    uint32 index;
};


END_CRUSTA

#endif //_TreeIndex_H_