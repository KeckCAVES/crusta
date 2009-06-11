#include <crusta/QuadCache.h>

#include <GL/GLContextData.h>
#include <Vrui/Vrui.h>

#include <crusta/DataManager.h>
#include <crusta/QuadTerrain.h>

BEGIN_CRUSTA

Cache crustaQuadCache(8192, 4096);

MainCache::
MainCache(uint size) :
    CacheUnit<MainCacheBuffer>(size), maxFetchRequests(16)
{
    //start the fetching thread
    fetchThread.start(this, &MainCache::fetchThreadFunc);
}

MainCache::
~MainCache()
{
    //stop the fetching thread
    fetchThread.cancel();
    fetchThread.join();
}

void MainCache::
request(const CacheRequests& reqs)
{
    //make sure merging of the requests is done one at a time
    Threads::Mutex::Lock lock(requestMutex);

    //requests cannot be duplicated so we simply append the requests
    childRequests.insert(childRequests.end(), reqs.begin(), reqs.end());

    //request a frame to process these requests
    if (!childRequests.empty())
        Vrui::requestUpdate();
}

void MainCache::
frame()
{
    //reprioritize the requests (do this here as the rest blocks fetching)
    std::sort(childRequests.begin(), childRequests.end());

#if 1
//inline version
    //giver the cache update 0.08ms (8ms for 120 fps, so 1% of frame time)
    double endTime = Vrui::getApplicationTime() + 0.08e-3;

    //update as much as possible
    for (CacheRequests::iterator it=childRequests.begin();
         Vrui::getApplicationTime()<endTime && it!=childRequests.end(); ++it)
    {
        FetchRequest fetch;
        fetch.parent          = it->parent;
        fetch.which           = it->child;
        TreeIndex childIndex  = fetch.parent->getData().index.down(fetch.which);
        fetch.child           = getBuffer(childIndex);
        if (fetch.child == NULL)
        {
            DEBUG_OUT(2,"MainCache::frame: no more room in the cache for ");
            DEBUG_OUT(2,"new data\n");
            fetch.child->invalidate();
            childRequests.clear();
            return;
        }
        fetch.child->touch();

        //fetch the data
        Crusta::getDataManager()->loadChild(
            fetch.parent, fetch.which, fetch.child);
    }
    childRequests.clear();


#else

//threaded version
    //block fetching as we manipulate the requests
    Threads::Mutex::Lock lock(requestMutex);

    //giver the cache update 0.08ms (8ms for 120 fps, so 1% of frame time)
    double endTime = Vrui::getApplicationTime() + 0.08e-3;

    //update as much as possible
    int numFetches = maxFetchRequests - static_cast<int>(fetchRequests.size());
    for (CacheRequests::iterator it=childRequests.begin();
         numFetches>0 && Vrui::getApplicationTime()<endTime &&
         it!=childRequests.end(); ++it)
    {
        //make sure the request is not pending
        FetchRequest searchReq;
        searchReq.parent = it->parent;
        searchReq.which  = it->child;
        if ((std::find(fetchResults.begin(), fetchResults.end(), searchReq) !=
             fetchResults.end()) ||
            std::find(fetchRequests.begin(), fetchRequests.end(), searchReq) !=
            fetchRequests.end())
        {
            continue;
        }

        FetchRequest fetch;
        fetch.parent = it->parent;
        fetch.which  = it->child;
        TreeIndex childIndex  = fetch.parent->getData().index.down(fetch.which);
        fetch.child = getBuffer(childIndex);
        if (fetch.child == NULL)
        {
            DEBUG_OUT(2,"MainCache::frame: no more room in the cache for ");
            DEBUG_OUT(2,"new data\n");
            childRequests.clear();
            return;
        }
        //pin the buffers we obtained
        fetch.parent->pin();
        fetch.child->pin();

        //submit the fetch request
        fetchRequests.push_back(fetch);
        fetchCond.signal();
        --numFetches;
    }
    childRequests.clear();

    //inject the processed requests into the tree
    for (FetchRequests::const_iterator it=fetchResults.begin();
         it!=fetchResults.end(); ++it)
    {
        it->parent->pin(false);
        it->parent->touch();
        it->child->pin(false);
        it->child->touch();

        DEBUG_OUT(1, "MainCache::frame: request for Index %s:%d processed\n",
                  it->parent->getData().index.med_str().c_str(), it->which);
    }
    fetchResults.clear();
#endif
}


MainCache::FetchRequest::
FetchRequest() :
    parent(NULL), child(NULL), which(~0)
{
}

bool MainCache::FetchRequest::
operator ==(const FetchRequest& other) const
{
    return parent->getData().index == other.parent->getData().index &&
           which == other.which;
}

bool MainCache::FetchRequest::
operator <(const FetchRequest& other) const
{
    return (size_t)(parent) < (size_t)(other.parent);
}

void* MainCache::
fetchThreadFunc()
{
    FetchRequest* fetch = NULL;
    while (true)
    {
        {
            Threads::Mutex::Lock lock(requestMutex);
            //post current result
            if (fetch != NULL)
            {
                fetchResults.push_back(*fetch);
                fetchRequests.pop_front();
            }
            //attempt to grab new request
            if (fetchRequests.empty())
                fetchCond.wait(requestMutex);
            fetch = &fetchRequests.front();
        }

        //fetch it
        Crusta::getDataManager()->loadChild(
            fetch->parent, fetch->which, fetch->child);
    }

    return NULL;
}


VideoCache::
VideoCache(uint size) :
    CacheUnit<VideoCacheBuffer>(size), streamBuffer(TILE_RESOLUTION)
{
}

VideoCacheBuffer* VideoCache::
getStreamBuffer()
{
    return &streamBuffer;
}


Cache::
Cache(uint mainSize, uint videoSize) :
    videoCacheSize(videoSize), mainCache(mainSize)
{
}


MainCache& Cache::
getMainCache()
{
    return mainCache;
}
VideoCache& Cache::
getVideoCache(GLContextData& contextData)
{
    GlData* glData = contextData.retrieveDataItem<GlData>(this);
    return glData->videoCache;
}


void Cache::
initContext(GLContextData& contextData) const
{
    GlData* glData = new GlData(videoCacheSize);
    contextData.addDataItem(this, glData);
}

Cache::GlData::
GlData(uint size) :
    videoCache(size)
{
}


END_CRUSTA
