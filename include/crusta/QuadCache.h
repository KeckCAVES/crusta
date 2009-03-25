#ifndef _QuadCache_H_
#define _QuadCache_H_

#ifdef __GNUC__
#include <tr1/unordered_map>
#else
#include <unordered_map>
#endif
#include <vector>

#include <GL/GLObject.h>
#include <Threads/Mutex.h>
#include <Threads/Thread.h>

#include <crusta/quadCommon.h>

namespace std
{
    using namespace tr1;
}

BEGIN_CRUSTA

/** underlying LRU cache functionality */
template <typename BufferType>
class CacheUnit
{
public:
    CacheUnit(uint size);
    ~CacheUnit();

    /** find a buffer within the cached set. Returns NULL if not found. */
    BufferType* findCached(const TreeIndex& index) const;
    /** request a buffer from the cache. A non-NULL buffer is returned as long
        as all the cache slots are not not pinned (either explicitly or because
        they are in use for the current frame). Optionally the location to a
        boolean can be passed and getBuffer will set if the buffer was already
        present or not at that location. */
    BufferType* getBuffer(const TreeIndex& index, bool* existed=NULL);


protected:
    typedef std::unordered_map<TreeIndex, BufferType*, TreeIndex::hash>
        BufferPtrMap;

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
    uint sortFrameNumber;
};

class MainCache : public CacheUnit<MainCacheBuffer>
{
public:
    MainCache(uint size);

    /** process requests */
    void frame();
    /** request data fetch/generation for a set of tree indices */
    void request(const MainCacheRequests& reqs);

protected:
    /** lazy thread function: process the non-critical part of data requests */
    void* lazyThreadFunc();

    /** keep track of pending critical requests */
    MainCacheRequests criticalRequests;
    /** keep track of lazy-fetch requests (critical data already loaded) */
    MainCacheRequests lazyRequests;

    /** used to protect access to the critical request queue */
    Threads::Mutex criticalMutex;
    /** used to protect access to the lazy request queue */
    Threads::Mutex lazyMutex;

    /** thread handling lazy request processing */
    Threads::Thread lazyThread;
};

class VideoCache : public CacheUnit<VideoCacheBuffer>
{
public:
    VideoCache(uint size);

    /** retrieve the temporary video buffer that can be used for streaming
        data */
    VideoCacheBuffer* getStreamBuffer();

protected:
    VideoCacheBuffer streamBuffer;
};

class Cache : public GLObject
{
public:
    Cache(uint mainSize, uint videoSize);

    /** retrieve the main memory cache unit */
    MainCache& getMainCache();
    /** retrieve the video memory cache unit */
    VideoCache& getVideoCache(GLContextData& contextData);

protected:
    /** needed during initContext, specified during construction */
    uint videoCacheSize;
    /** the main memory cache unit */
    MainCache mainCache;

//- inherited from GLObject
public:
   	virtual void initContext(GLContextData& contextData) const;

protected:
    struct GlData : public GLObject::DataItem
    {
        GlData(uint size);

        /** the video memory cache unit for the GL context */
        VideoCache videoCache;
    };
};

extern Cache crustaQuadCache;

END_CRUSTA

#include <crusta/QuadCache.hpp>

#endif //_QuadCache_H_
