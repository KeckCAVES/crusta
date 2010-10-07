#include <crusta/QuadCache.h>

#include <GL/GLContextData.h>
#include <Vrui/Vrui.h>

#include <crusta/CrustaSettings.h>
#include <crusta/DataManager.h>


BEGIN_CRUSTA


Cache::
Cache()
{
    //initialize all the main memory caches
    mainCache.node.init(SETTINGS->cacheMainNodeSize);
    mainCache.geometry.init(SETTINGS->cacheMainGeometrySize, TILE_RESOLUTION);
    mainCache.height.init(SETTINGS->cacheMainHeightSize, TILE_RESOLUTION);
    mainCache.imagery.init(SETTINGS->cacheMainImagerySize, TILE_RESOLUTION);
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
    GlData*   glData   = new GlData;
    GpuCache& gpuCache = glData->gpuCache;

    //initialize all the gpu memory caches
    gpuCache.geometry.init(SETTINGS->cacheGpuGeometrySize, TILE_RESOLUTION,
                           GL_RGB32F_ARB, GL_LINEAR);
    gpuCache.height.init(SETTINGS->cacheGpuHeightSize, TILE_RESOLUTION,
                         GL_INTENSITY32F_ARB, GL_LINEAR);
    gpuCache.imagery.init(SETTINGS->cacheGpuImagerySize, TILE_RESOLUTION,
                          GL_RGB, GL_LINEAR);
    gpuCache.lineData.init(
        SETTINGS->cacheGpuLineDataSize, SETTINGS->lineDataTexSize,
        GL_RGBA32F_ARB, GL_LINEAR);
    gpuCache.coverage.init(
        SETTINGS->cacheGpuCoverageSize, SETTINGS->lineCoverageTexSize,
        GL_LUMINANCE_ALPHA, GL_NEAREST);

    contextData.addDataItem(this, glData);
}


Cache* CACHE;


END_CRUSTA
