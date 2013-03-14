#ifndef _DataIndex_H_
#define _DataIndex_H_


#include <crustacore/TreeIndex.h>


BEGIN_CRUSTA

/** uniquely specifies a data element for use in caching */
struct DataIndex : public TreeIndex
{
    struct hash {
        size_t operator() (const DataIndex& i) const;
    };

    DataIndex();
    DataIndex(uint8_t dataId, const TreeIndex& treeIndex);
    DataIndex(const DataIndex& i);
    
    DataIndex& operator=(const DataIndex& other);
    bool operator==(const DataIndex& other) const;    
    
    uint8_t getDataId() const;
    TreeIndex getTreeIndex() const;

    static const DataIndex invalid;
};


END_CRUSTA


#endif //_DataIndex_H_
