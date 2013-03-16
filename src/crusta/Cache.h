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

#include <crusta/DataIndex.h>

#include <crusta/vrui.h>

#ifdef __GNUC__
    #if __GNUC__ > 3 && __GNUC_MINOR__ > 0
        #define PortableTable std::tr1::unordered_map
    #else
        #define PortableTable __gnu_cxx::hash_map
    #endif
#else
    #define PortableTable stdext::hash_map
#endif


namespace crusta {


template <typename BufferParam>
class CacheUnit;

template <typename DataParam>
class CacheBufferBase
{
public:
    typedef DataParam DataType;

    CacheBufferBase();

    /** retrieve the frame stamp of the buffer */
    const FrameStamp& getFrameStamp() const;
    /** retrieve the index of the buffer */
    const DataIndex& getIndex() const;
    /**\{ retrieve the data from the buffer */
    DataParam& getData();
    const DataParam& getData() const;
    /**\}*/

protected:
    static const FrameStamp OLDEST_FRAMESTAMP;

    struct State
    {
        uint8_t grabbed : 1;
        uint8_t valid   : 1;
        uint8_t pinned  : 6;
    };

    /** state of the buffer */
    State state;
    /** frame stamp used to evaluate LRU prioritization */
    FrameStamp frameStamp;
    /** unique key for the data entry */
    DataIndex index;

    /** the actual node data */
    DataParam data;
};

template <typename DataParam>
class CacheBuffer : public CacheBufferBase<DataParam>
{
    friend class CacheUnit< CacheBuffer<DataParam> >;

public:
    typedef std::list<CacheBuffer*>    LruList;
    typedef typename LruList::iterator LruHandle;

protected:
    /** handle of the buffer in the LRU control container */
    LruHandle lruHandle;
};

template <typename DataParam>
class CacheArrayBuffer : public CacheBufferBase<DataParam*>
{
    friend class  CacheUnit< CacheArrayBuffer<DataParam> >;

public:
    typedef DataParam*                   DataType;
    typedef DataParam                    DataArrayType;
    typedef std::list<CacheArrayBuffer*> LruList;
    typedef typename LruList::iterator   LruHandle;

    ~CacheArrayBuffer();

protected:
    /** handle of the buffer in the LRU control container */
    LruHandle lruHandle;
};


/** underlying LRU cache functionality */
template <typename BufferParam>
class CacheUnit
{
public:
    typedef BufferParam BufferType;

    virtual ~CacheUnit();

    /** initialize the cache */
    void init(const std::string& iName, int size);
    /** initialize the data of the buffers */
    virtual void initData(typename BufferParam::DataType& data);

    /** reset the unit, unpinning and invalidating all the current entries */
    void clear();

    /** check to see if the buffer is valid */
    bool isValid(const BufferParam* const buffer) const;
    /** check to see if the buffer is pinned */
    bool isPinned(const BufferParam* const buffer) const;
    /** check to see if the buffer is grabbed (removed from the cache) */
    bool isGrabbed(const BufferParam* const buffer) const;
    /** check to see if the buffer has been touched in this frame */
    bool isCurrent(const BufferParam* const buffer) const;

    /** confirm use of the buffer for the current frame */
    void touch(BufferParam* buffer);
    /** pin the element in the cache such that it cannot be swaped out */
    void pin(BufferParam* buffer);
    /** unpin the element in the cache */
    void unpin(BufferParam* buffer);

    /** find a buffer within the cached set. Returns NULL if not found. */
    BufferParam* find(const DataIndex& index) const;
    /** request a buffer from the cache. The least recently used that is not a
        pinned buffer is returned. An additional filter may limit available
        buffer to ones that are older than 'older'. The buffer is removed from
        the cache as a result.
        WARNING: it is assumed that a call to find was issued prior.
                 grabBuffer does not verify that an appropriate buffer is
                 already cached. */
    BufferParam* grabBuffer(const FrameStamp older);
    /** return a buffer to the cache. The buffer will be reinserted into the
        cache with the given index */
    void releaseBuffer(const DataIndex& index, BufferParam* buffer);

    /** age the given number of most recently used entries */
    void ageMRU(int numBuffers, const FrameStamp age);

///\todo move back to the protected group
    /** prints the content of the cache in LRU order */
    void printCache();

protected:
    typedef PortableTable<DataIndex,BufferParam*,DataIndex::hash> BufferPtrMap;
    typedef typename BufferParam::LruList LruList;

    /** updates buffers to reflect having been touched. (internal use, locks are
        left to the calling method) */
    void touchBuffer(BufferParam* buffer);

    /** prints the state of the LRU */
    void printLru(const char* cause);

    /** a string identifier for the cache (mainly used for debugging) */
    std::string name;

    /** keep a record of all the buffers cached by the unit */
    BufferPtrMap cached;
    /** keep a LRU prioritized view of the cached buffers */
    LruList lru;

    /** synchronize access to the cache */
    Threads::Mutex cacheMutex;
};


} //namespace crusta


#include <crusta/Cache.hpp>


#endif //_Cache_H_
