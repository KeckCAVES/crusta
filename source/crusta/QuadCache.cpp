#include <crusta/QuadCache.h>

#include <GL/GLContextData.h>
#include <Vrui/Vrui.h>

#include <crusta/DataManager.h>
#include <crusta/QuadTerrain.h>

BEGIN_CRUSTA

MainCache::Request::
Request() :
    lod(0), parent(NULL), child(~0)
{}

MainCache::Request::
Request(float iLod, MainCacheBuffer* iParent, uint8 iChild) :
    lod(iLod), parent(iParent), child(iChild)
{}

bool MainCache::Request::
operator ==(const Request& other) const
{
    /**\todo there is some danger here. Double check that I'm really only discarding
     components of the request that are allowed to be different. So far, since
     everything is fetched on a node basis the index is all that is needed to
     coalesce requests */
    return parent->getData().index == other.parent->getData().index &&
    child == other.child;
}
bool MainCache::Request::
operator <(const Request& other) const
{
    return Math::abs(lod) < Math::abs(other.lod);
}

MainCache::
MainCache(uint size, Crusta* iCrusta) :
    CacheUnit<MainCacheBuffer>(size, iCrusta), maxFetchRequests(16)
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
request(const Request& req)
{
    //make sure merging of the requests is done one at a time
    Threads::Mutex::Lock lock(requestMutex);

    //requests cannot be duplicated so we simply append the requests
    childRequests.push_back(req);

    //request a frame to process these requests
    if (!childRequests.empty())
        Vrui::requestUpdate();
}

void MainCache::
request(const Requests& reqs)
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

#if 0
//inline version
    //giver the cache update 0.08ms (8ms for 120 fps, so 1% of frame time)
    double endTime = Vrui::getApplicationTime() + 0.08e-3;

    //update as much as possible
    for (Requests::iterator it=childRequests.begin();
         Vrui::getApplicationTime()<endTime && it!=childRequests.end(); ++it)
    {
        FetchRequest fetch;
        fetch.parent          = it->parent;
        fetch.which           = it->child;
        TreeIndex childIndex  = fetch.parent->getData().index.down(fetch.which);
        fetch.child           = getBuffer(childIndex);
        if (fetch.child == NULL)
        {
CRUSTA_DEBUG_OUT(2,"MainCache::frame: no more room in the cache for ");
CRUSTA_DEBUG_OUT(2,"new data\n");
            fetch.child->invalidate();
            childRequests.clear();
            return;
        }
        fetch.child->touch();

        //fetch the data
        crusta->getDataManager()->loadChild(
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
    for (Requests::iterator it=childRequests.begin();
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
CRUSTA_DEBUG_OUT(2,"MainCache::frame: no more room in the cache for ");
CRUSTA_DEBUG_OUT(2,"new data\n");
            childRequests.clear();
            return;
        }
        //pin the buffers we obtained
        pin(fetch.parent);
        pin(fetch.child, false);

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
        unpin(it->parent);
        touch(it->parent);
        unpin(it->child);
        touch(it->child);

CRUSTA_DEBUG_OUT(1, "MainCache::frame: request for Index %s:%d processed\n",
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
                //need to reactivate cache to process fetch completion
                Vrui::requestUpdate();
            }
            //attempt to grab new request
            if (fetchRequests.empty())
                fetchCond.wait(requestMutex);
            fetch = &fetchRequests.front();
        }

        //fetch it
        crusta->getDataManager()->loadChild(
            fetch->parent, fetch->which, fetch->child);
    }

    return NULL;
}


VideoCache::
VideoCache(uint size, Crusta* iCrusta) :
    CacheUnit<VideoCacheBuffer>(size, iCrusta), streamBuffer(TILE_RESOLUTION)
{
}

VideoCacheBuffer* VideoCache::
getStreamBuffer()
{
    return &streamBuffer;
}


Cache::
Cache(uint mainSize, uint videoSize, Crusta* iCrusta) :
    videoCacheSize(videoSize), crustaInstance(iCrusta),
    mainCache(mainSize, iCrusta)
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
    GlData* glData = new GlData(videoCacheSize, crustaInstance);
    contextData.addDataItem(this, glData);
}

Cache::GlData::
GlData(uint size, Crusta* crustaInstance) :
    videoCache(size, crustaInstance)
{
}


END_CRUSTA
