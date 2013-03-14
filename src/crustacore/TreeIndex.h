#ifndef _TreeIndex_H_
#define _TreeIndex_H_

#include <string>

#include <crustacore/basics.h>

BEGIN_CRUSTA

/** uniquely specifies a node in the quadtree hierarchy */
struct TreeIndex
{
    TreeIndex(uint8_t iPatch=0, uint8_t iChild=0, uint8_t iLevel=0, uint64_t iIndex=0);
    TreeIndex(const TreeIndex& i);

    TreeIndex& operator=(const TreeIndex& other);
    bool operator==(const TreeIndex& other) const;
    bool operator!=(const TreeIndex& other) const;

    ///\{ reserved for differentiating same indices (5 bits)
    uint8_t reserved() const;
    void  reserved(const uint8_t v);
    ///\}
    ///\{ index of the base patch of the global hierarchy (5 bits)
    uint8_t patch() const;
    void  patch(const uint8_t v);
    ///\}
    ///\{ index within the group of siblings (2 bits)
    uint8_t child() const;
    void  child(const uint8_t v);
    ///\}
    ///\{ level in the global hierarchy: 0 is root (6 bits)
    uint8_t level() const;
    void  level(const uint8_t v);
    ///\}
    /**\{ describes a path from the root to the indicated node as a sequence
          of two-bit child-indices. The sequence starts with the least
          significant bits. (46 bits)*/
    uint64_t index() const;
    void   index(const uint64_t v);
    ///\}

    TreeIndex up() const;
    TreeIndex down(uint8_t which) const;

    std::string str() const;
    std::string med_str() const;
    friend std::ostream& operator<<(std::ostream& os, const TreeIndex& i);

    static const TreeIndex invalid;

    /** the raw bits of the index. Use accessors to read/write components */
    uint64_t raw;
};

/** A traversal helper for following the path from the root to a node specified
    by the tree index provided during construction. Only a forward traversal is
    supported */
struct TreePath
{
    static const uint8_t END = ~0x0;

    TreePath(TreeIndex i);

    /** returns the index of the child to go to for continueing the traversal.
     The end is reached when the returned child index is END */
    uint8_t pop();

    uint8_t level;
    uint64_t index;
};


END_CRUSTA

#endif //_TreeIndex_H_
