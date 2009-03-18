#include <QuadCache.h>

#include <GL/GLContextData.h>
#include <Vrui/Vrui.h>

#include <QuadTerrain.h>

BEGIN_CRUSTA

Cache crustaQuadCache(1024, 512);

MainCache::
MainCache(uint size) :
    CacheUnit<MainCacheBuffer>(size)
{
}

void MainCache::
request(const MainCacheRequests& reqs)
{
    //make sure merging of the requests is done one at a time
    Threads::Mutex::Lock lock(criticalMutex);

    //add in all the requests without duplicating them
    for (MainCacheRequests::const_iterator it=reqs.begin();it!=reqs.end();++it)
    {
        MainCacheRequests::iterator searched =
            std::find(criticalRequests.begin(), criticalRequests.end(),
                      MainCacheRequest(0, it->index, Scope()));
        if (searched != criticalRequests.end())
            searched->lod = std::max(searched->lod, it->lod);
        else
            criticalRequests.push_back(*it);
    }

    //reprioritize the requests
    std::sort(criticalRequests.begin(), criticalRequests.end());
}

void MainCache::
frame()
{
    //giver the cache update 0.08ms (8ms for 120 fps, so 1% of frame time)
    double endTime = Vrui::getApplicationTime() + 0.08e-5;//0.08e-3;

    //update as much as possible
    for (MainCacheRequests::iterator it=criticalRequests.begin();
         it!=criticalRequests.end() && Vrui::getApplicationTime()<endTime; ++it)
    {
        MainCacheBuffer* buf =
            crustaQuadCache.getMainCache().getBuffer(it->index);
        if (buf == NULL)
        {
            DEBUG_OUT(2, "MainCache::frame: no more room in the cache for ");
            DEBUG_OUT(2, "new data\n");
            break;
        }
        buf->touch(frameNumber);
        
        //process the critical data for that request
///\todo currently processes all data
        const QuadNodeMainData& data = buf->getData();
        QuadTerrain::generateGeometry(it->scope, data.geometry);
        QuadTerrain::generateHeight(data.geometry, data.height);
        QuadTerrain::generateColor(data.height, data.color);
        DEBUG_OUT(5, "MainCache::frame: request for Index %s processed\n",
                  it->index.str().c_str());
    }

    criticalRequests.clear();
    ++frameNumber;
}


void* MainCache::
lazyThreadFunc()
{
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
