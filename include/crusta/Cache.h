#ifndef _Cache_H_
#define _Cache_H_

#ifdef __GNUC__
    #if __GNUC__ > 3 && __GNUC_MINOR__ > 0
        #include <tr1/unordered_map>
        /* to get around the inclusion of <cmath> from above undef'ing all the
           <math.h> macros and defining corresponding functions in std:: */
        using namespace std;
    #else
        #include <ext/hash_map>
    #endif
#else
    #include <hash_map>
#endif

#include <list>
#include <vector>

#include <crusta/CrustaComponent.h>
#include <crusta/TreeIndex.h>

#ifdef __GNUC__
    #if __GNUC__ > 3 && __GNUC_MINOR__ > 0
        #define PortableTable std::tr1::unordered_map
    #else
        #define PortableTable __gnu_cxx::hash_map
    #endif
#else
    #define PortableTable stdext::hash_map
#endif


BEGIN_CRUSTA


template <typename BufferType>
class CacheUnit;

template <typename NodeDataType>
class CacheBuffer
{
    friend class CacheUnit< CacheBuffer<NodeDataType> >;

public:
    CacheBuffer(uint size);

    /** retrieve the main memory node data from the buffer */
    NodeDataType& getData();
    const NodeDataType& getData() const;

    /** query the frame number of the buffer */
    FrameNumber getFrameNumber() const;

protected:
    /** sequence number used to evaluate LRU prioritization */
    FrameNumber frameNumber;
    /** the actual node data */
    NodeDataType data;
};


/** underlying LRU cache functionality */
template <typename BufferType>
class CacheUnit : public CrustaComponent
{
public:
    CacheUnit(uint size, Crusta* iCrusta);
    ~CacheUnit();

    /** find a buffer within the cached set. Returns NULL if not found. */
    BufferType* findCached(const TreeIndex& index) const;
    /** request a buffer from the cache. A non-NULL buffer is returned as long
        as all the cache slots are not not pinned (either explicitly or because
        they are in use for the current frame). Optionally the location to a
        boolean can be passed and getBuffer will set if the buffer was already
        present or not at that location. */
    BufferType* getBuffer(const TreeIndex& index, bool* existed=NULL);

    /** check to see if the buffer is active in the current frame */
    bool isActive(const BufferType* const buffer) const;
    /** check to see if the buffer is valid */
    bool isValid(const BufferType* const buffer) const;
    /** confirm use of the buffer for the current frame */
    void touch(BufferType* buffer) const;
    /** invalidate a buffer */
    void invalidate(BufferType* buffer) const;
    /** pin the element in the cache such that it cannot be swaped out */
    void pin(BufferType* buffer, bool asUsable=true)const;
    /** unpin the element in the cache (resets it to 0 frametime) */
    void unpin(BufferType* buffer)const;

protected:
    typedef PortableTable<TreeIndex, BufferType*, TreeIndex::hash> BufferPtrMap;

    /** combines a tree index with a cache buffer when the buffer is taken
        outside the context of the buffer map */
    struct IndexedBuffer
    {
        IndexedBuffer(TreeIndex iIndex, BufferType* iBuffer);
        bool operator >(const IndexedBuffer& other) const;

        TreeIndex   index;
        BufferType* buffer;
    };
    typedef std::vector<IndexedBuffer> IndexedBuffers;

    /** make sure the LRU prioritized sequence of the cached buffers is up to
        date*/
    void refreshLru();

    /** keep a record of all the buffers cached by the unit */
    BufferPtrMap cached;
    /** keep a LRU prioritized view of the cached buffers */
    IndexedBuffers lruCached;

    /** frame number when the last prioritization of the chached set was
        done */
    FrameNumber sortFrameNumber;
};


END_CRUSTA


#include <crusta/Cache.hpp>


#endif //_Cache_H_
