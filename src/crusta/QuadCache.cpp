#include <crusta/QuadCache.h>

#include <crusta/CrustaSettings.h>
#include <crusta/DataManager.h>

#include <crusta/vrui.h>


namespace crusta {


Cache::
Cache() :
    clearStamp(0)
{
    //initialize all the main memory caches
    mainCache.node.init("MainNode", SETTINGS->cacheMainNodeSize);
    mainCache.geometry.init("MainGeometry", SETTINGS->cacheMainGeometrySize,
                            TILE_RESOLUTION);
    mainCache.color.init("MainColor", SETTINGS->cacheMainColorSize,
                            TILE_RESOLUTION);
    mainCache.layerf.init("MainLayerf", SETTINGS->cacheMainLayerfSize,
                          TILE_RESOLUTION);
}


void Cache::
clear()
{
    mainCache.node.clear();
    mainCache.geometry.clear();
    mainCache.color.clear();
    mainCache.layerf.clear();

    clearStamp = CURRENT_FRAME;
}


void Cache::
display(GLContextData& contextData)
{
    GlData* glData = contextData.retrieveDataItem<GlData>(this);
    if (glData->clearStamp < clearStamp)
    {
        GpuCache& gpuCache = glData->gpuCache;
        gpuCache.geometry.clear();
        gpuCache.color.clear();
        gpuCache.layerf.clear();
        gpuCache.coverage.clear();
        gpuCache.lineData.clear();

        glData->clearStamp = clearStamp;
    }
}


MainCache& Cache::
getMainCache()
{
    return mainCache;
}

GpuCache& Cache::
getGpuCache(GLContextData& contextData)
{
    GlData* glData = contextData.retrieveDataItem<GlData>(this);
    return glData->gpuCache;
}


void Cache::
initContext(GLContextData& contextData) const
{
    GlData* glData = new GlData;
    glData->clearStamp = 0;

    GpuCache& gpuCache = glData->gpuCache;

    //initialize all the gpu memory caches
    gpuCache.geometry.init("GpuGeometry", SETTINGS->cacheGpuGeometrySize,
                           TILE_RESOLUTION, GL_RGB32F_ARB, GL_LINEAR);
    gpuCache.color.init("GpuColor", SETTINGS->cacheGpuColorSize,
                        TILE_RESOLUTION, GL_RGB, GL_LINEAR);
    gpuCache.layerf.init("GpuLayerf", SETTINGS->cacheGpuLayerfSize,
                         TILE_RESOLUTION, GL_INTENSITY32F_ARB, GL_LINEAR);
    gpuCache.lineData.init("GpuLineData", SETTINGS->cacheGpuLineDataSize,
                           SETTINGS->lineDataTexSize,
                           GL_RGBA32F_ARB, GL_LINEAR);
    gpuCache.coverage.init("GpuCoverage", SETTINGS->cacheGpuCoverageSize,
                           SETTINGS->lineCoverageTexSize,
                           GL_RG, GL_NEAREST);

    contextData.addDataItem(this, glData);
}


Cache* CACHE;


} //namespace crusta
