#include <crusta/QuadCache.h>

#include <GL/GLContextData.h>
#include <Vrui/Vrui.h>

#include <crusta/QuadTerrain.h>

BEGIN_CRUSTA

Cache crustaQuadCache(8192, 4096);

MainCache::
MainCache(uint size) :
    CacheUnit<MainCacheBuffer>(size), maxFetchRequests(3)
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

//inline version
#if 1
    //giver the cache update 0.08ms (8ms for 120 fps, so 1% of frame time)
    double endTime = Vrui::getApplicationTime() + 0.08e-3;

    //update as much as possible
    for (CacheRequests::iterator it=childRequests.begin();
         Vrui::getApplicationTime()<endTime && it!=childRequests.end(); ++it)
    {
        FetchRequest fetch;
        fetch.node = it->node;
        //first get the cache buffers required, as this might fail (cache full)
        for (int i=0; i<4; ++i)
        {
            TreeIndex childIndex  = fetch.node->index.down(i);
            fetch.childBuffers[i] = getBuffer(childIndex,&fetch.childCached[i]);
            if (fetch.childBuffers[i] == NULL)
            {
                DEBUG_OUT(2,"MainCache::frame: no more room in the cache for ");
                DEBUG_OUT(2,"new data\n");
                for (int j=i-1; j>=0; --j)
                    fetch.childBuffers[j]->invalidate();
                childRequests.clear();
                return;
            }
            fetch.childBuffers[i]->touch();
        }
        //allocate the new children
        fetch.children = fetch.node->terrain->grabNodeBlock();

        //fetch the data
        fetch.node->terrain->loadChildren(
            fetch.node, fetch.children, fetch.childBuffers, fetch.childCached);
        //connect it all up
        assert(fetch.node->children == NULL);
        fetch.node->children = fetch.children;
DEBUG_BEGIN(1)
fetch.node->terrain->checkTree(fetch.node);
DEBUG_END
    }
    childRequests.clear();
#endif

//threaded version
#if 0
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
        searchReq.node = it->node;
        if ((std::find(fetchResults.begin(), fetchResults.end(), searchReq) !=
             fetchResults.end()) ||
            std::find(fetchRequests.begin(), fetchRequests.end(), searchReq) !=
            fetchRequests.end())
        {
            continue;
        }

        FetchRequest fetch;
        fetch.node = it->node;
        //first get the cache buffers required, as this might fail (cache full)
        for (int i=0; i<4; ++i)
        {
            TreeIndex childIndex  = fetch.node->index.down(i);
            fetch.childBuffers[i] = getBuffer(childIndex,&fetch.childCached[i]);
            if (fetch.childBuffers[i] == NULL)
            {
                DEBUG_OUT(2,"MainCache::frame: no more room in the cache for ");
                DEBUG_OUT(2,"new data\n");
                childRequests.clear();
                return;
            }
        }
        //pin the buffers we obtained
        for (int i=0; i<4; ++i)
            fetch.childBuffers[i]->pin();

        //allocate the new children
        fetch.children = fetch.node->terrain->grabNodeBlock();

        //submit the fetch request
        fetch.node->pinned = true;
        fetchRequests.push_back(fetch);
        fetchCond.signal();
        --numFetches;
    }
    childRequests.clear();

    //inject the processed requests into the tree
    for (FetchRequests::const_iterator it=fetchResults.begin();
         it!=fetchResults.end(); ++it)
    {
        for (int i=0; i<4; ++i)
            it->children[i].mainBuffer->pin(false);
        assert(it->node->children == NULL);
        it->node->children = it->children;
        it->node->pinned = false;
DEBUG_BEGIN(1)
it->node->terrain->checkTree(it->node);
DEBUG_END

        DEBUG_OUT(1, "MainCache::frame: request for Index %s processed\n",
                  it->node->index.med_str().c_str());
    }
    fetchResults.clear();
#endif
}


MainCache::FetchRequest::
FetchRequest() :
    node(NULL), children(NULL)
{
    for (int i=0; i<4; ++i)
    {
        childBuffers[i] = NULL;
        childCached[i]  = false;
    }
}

MainCache::FetchRequest::
FetchRequest(const FetchRequest& other) :
    node(other.node), children(other.children)
{
    for (int i=0; i<4; ++i)
    {
        childBuffers[i] = other.childBuffers[i];
        childCached[i]  = other.childCached[i];
    }
}

bool MainCache::FetchRequest::
operator ==(const FetchRequest& other) const
{
    return node == other.node;
}

bool MainCache::FetchRequest::
operator <(const FetchRequest& other) const
{
    return (size_t)(node) < (size_t)(other.node);
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
        fetch->node->terrain->loadChildren(
            fetch->node,fetch->children,fetch->childBuffers,fetch->childCached);
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
