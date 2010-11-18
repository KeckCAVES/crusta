#ifndef _QuadCache_H_
#define _QuadCache_H_


#include <GL/GLObject.h>
#include <Threads/Cond.h>
#include <Threads/Mutex.h>
#include <Threads/Thread.h>

#include <crusta/Cache.h>
#include <crusta/QuadNodeData.h>


BEGIN_CRUSTA

typedef CacheBuffer<QuadNodeMainData>    MainCacheBuffer;
typedef CacheBuffer<QuadNodeVideoData>   VideoCacheBuffer;
typedef CacheBuffer<QuadNodeGpuLineData> GpuLineCacheBuffer;


class MainCache : public CacheUnit<MainCacheBuffer>
{
public:
    /** information required to process the fetch/generation of data */
    class Request
    {
        friend class MainCache;

    public:
        Request();
        Request(float iLod, MainCacheBuffer* iParent, uint8 iChild);

        bool operator ==(const Request& other) const;
        bool operator <(const Request& other) const;

    protected:
        /** lod value used for prioritizing the requests */
        float lod;
        /** pointer to the parent of the requested */
        MainCacheBuffer* parent;
        /** index of the child to be loaded */
        uint8 child;
    };

    typedef std::vector<Request> Requests;

    MainCache(uint size, Crusta* iCrusta);
    ~MainCache();

    /** process requests */
    void frame();
    /** request data fetch/generation for a node */
    void request(const Request& req);
    /** request data fetch/generation for a set of tree indices */
    void request(const Requests& reqs);

    /** reset the unit, unpinning and invalidating all the current entries */
    void reset();

protected:
    struct FetchRequest
    {
        MainCacheBuffer* parent;
        MainCacheBuffer* child;
        uint8            which;

        FetchRequest();
        bool operator ==(const FetchRequest& other) const;
        bool operator <(const FetchRequest& other) const;
    };
    typedef std::list<FetchRequest> FetchRequests;

    /** fetch thread function: process the generation/reading of the data */
    void* fetchThreadFunc();

    /** keep track of pending child requests */
    Requests childRequests;

    /** impose a limit on the number of outstanding fetch requests. This
        minimizes processing outdated requests */
    int maxFetchRequests;
    /** buffer for fetch requests to the fetch thread */
    FetchRequests fetchRequests;
    /** buffer for fetch results that have been processed by the fetch thread */
    FetchRequests fetchResults;

    /** used to protect access to any of the buffers. For simplicity fetching
        is stalled as a frame is processed */
    Threads::Mutex requestMutex;
    /** allow the fetching thread to blocking wait for requests */
    Threads::Cond fetchCond;

    /** thread handling fetch request processing */
    Threads::Thread fetchThread;
};

class VideoCache : public CacheUnit<VideoCacheBuffer>
{
public:
    VideoCache(uint size, Crusta* iCrusta);

    /** retrieve the temporary video buffer that can be used for streaming
        data */
    VideoCacheBuffer* getStreamBuffer();

protected:
    VideoCacheBuffer streamBuffer;
};

class GpuLineCache : public CacheUnit<GpuLineCacheBuffer>
{
public:
    GpuLineCache(uint size, Crusta* iCrusta);

    /** retrieve the temporary video buffer that can be used for streaming
        data */
    GpuLineCacheBuffer* getStreamBuffer();

protected:
    GpuLineCacheBuffer streamBuffer;
};

class Cache : public GLObject
{
public:
    Cache(uint mainSize, uint videoSize, uint gpuLineSize, Crusta* iCrusta);

    /** retrieve the main memory cache unit */
    MainCache& getMainCache();
    /** retrieve the video memory cache unit */
    VideoCache& getVideoCache(GLContextData& contextData);
    /** retrieve the gpu line data cache unit */
    GpuLineCache& getGpuLineCache(GLContextData& contextData);

protected:
    /** needed during initContext, specified during construction */
    uint videoCacheSize;
    /** needed during initContext, specified during construction */
    uint gpuLineCacheSize;
    /** needed during initContext, specified during construction */
    Crusta* crustaInstance;
    /** the main memory cache unit */
    MainCache mainCache;

//- inherited from GLObject
public:
    virtual void initContext(GLContextData& contextData) const;

protected:
    struct GlData : public GLObject::DataItem
    {
        GlData(uint videoSize, uint lineSize, Crusta* crustaInstance);

        /** the video memory cache unit for the GL context */
        VideoCache videoCache;
        /** the gpu line data cache unit for the GL context */
        GpuLineCache lineCache;
    };
};


END_CRUSTA


#endif //_QuadCache_H_
