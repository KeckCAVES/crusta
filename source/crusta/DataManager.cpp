#include <crusta/DataManager.h>

#include <sstream>

#include <Math/Constants.h>

#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>
#include <crusta/PixelOps.h>
#include <crusta/PolyhedronLoader.h>
#include <crusta/QuadCache.h>
#include <crusta/QuadTerrain.h>
#include <crusta/Triacontahedron.h>


BEGIN_CRUSTA


DataManager::GlData* DataManager::glData = NULL;


DataManager::Request::
Request() :
    crusta(NULL), lod(0), child(~0)
{
}

DataManager::Request::
Request(Crusta* iCrusta, float iLod, const NodeMainBuffer& iParent,
        uint8 iChild) :
    crusta(iCrusta), lod(iLod), parent(iParent), child(iChild)
{
}

bool DataManager::Request::
operator ==(const Request& other) const
{
    /**\todo there is some danger here. Double check that I'm really only discarding
       components of the request that are allowed to be different. So far, since
       everything is fetched on a node basis the index is all that is needed to
       coalesce requests */
    return parent.node->getData().index==other.parent.node->getData().index &&
           child==other.child;
}

bool DataManager::Request::
operator >(const Request& other) const
{
    return Math::abs(lod) > Math::abs(other.lod);
}


DataManager::
DataManager() :
demFile(NULL), colorFile(NULL), polyhedron(NULL),
demNodata(0), colorNodata(128, 128, 128), clearGpuCachesStamp(0)
{
    tempGeometryBuf = new double[TILE_RESOLUTION*TILE_RESOLUTION*3];
    glData = new GlData;
}

DataManager::
~DataManager()
{
    unload();

    delete tempGeometryBuf;
    delete glData;
}

void DataManager::
load(const std::string& demPath, const std::string& colorPath)
{
    //detach from existing databases
    unload();

    if (!demPath.empty())
    {
        demFile = new DemFile;
        try
        {
            demFile->open(demPath);
            polyhedron = PolyhedronLoader::load(demFile->getPolyhedronType(),
                                                SETTINGS->globeRadius);
            demNodata  = demFile->getNodata();
        }
        catch (std::runtime_error e)
        {
            delete demFile;
            demFile = NULL;
            demNodata = GlobeData<DemHeight>::defaultNodata();

            std::cerr << e.what();
        }
    }
    else
        demNodata = GlobeData<DemHeight>::defaultNodata();

    if (!colorPath.empty())
    {
        colorFile = new ColorFile;
        try
        {
            colorFile->open(colorPath);
            colorNodata = colorFile->getNodata();

            if (polyhedron != NULL)
            {
                if (polyhedron->getType() != colorFile->getPolyhedronType())
                {
                    std::cerr << "Mismatching polyhedron for " <<
                                 demPath.c_str() << " and " <<
                                 colorPath.c_str() << ".\n";

                    delete colorFile;
                    colorFile = NULL;
                    colorNodata = GlobeData<TextureColor>::defaultNodata();
                }
            }
            else
            {
                polyhedron =
                    PolyhedronLoader::load(colorFile->getPolyhedronType(),
                                           SETTINGS->globeRadius);
            }

        }
        catch (std::runtime_error e)
        {
            delete colorFile;
            colorFile = NULL;
            colorNodata = GlobeData<TextureColor>::defaultNodata();

            std::cerr << e.what();
        }
    }
    else
        colorNodata = GlobeData<TextureColor>::defaultNodata();

    if (polyhedron == NULL)
        polyhedron = new Triacontahedron(SETTINGS->globeRadius);

    //start the fetching thread
    fetchThread.start(this, &DataManager::fetchThreadFunc);
}

void DataManager::
unload()
{
    //stop the fetching thread
    if (!fetchThread.isJoined())
    {
        fetchThread.cancel();
        fetchThread.join();
    }

    //clear all the requests
    childRequests.clear();
    fetchRequests.clear();
    fetchResults.clear();

    //clear the main memory caches and flag the GPU ones
    MainCache& mc = CACHE->getMainCache();
    mc.node.clear();
    mc.geometry.clear();
    mc.height.clear();
    mc.imagery.clear();

    clearGpuCachesStamp = CURRENT_FRAME;

    if (demFile)
    {
        delete demFile;
        demFile = NULL;
    }
    if (colorFile)
    {
        delete colorFile;
        colorFile = NULL;
    }
    if (polyhedron)
    {
        delete polyhedron;
        polyhedron = NULL;
    }
}


bool DataManager::
hasDemData() const
{
    return demFile != NULL;
}

bool DataManager::
hasColorData() const
{
    return colorFile != NULL;
}


const Polyhedron* const DataManager::
getPolyhedron() const
{
    return polyhedron;
}

const DemHeight& DataManager::
getDemNodata()
{
    return demNodata;
}

const TextureColor& DataManager::
getColorNodata()
{
    return colorNodata;
}


#define GRAB_BUFFER(cacheType, name, cache, index)\
cacheType::BufferType* name##Buf = cache.find(index);\
if (name##Buf == NULL)\
    name##Buf = cache.grabBuffer(false);\
if (name##Buf == NULL)\
   {\
       Misc::throwStdErr("DataManager::loadRoot: failed to acquire %s cache "\
                         "buffer for root node of patch %d", #name,\
                         rootIndex.patch);\
   }\
cacheType::BufferType::DataType& name##Data = name##Buf->getData()


void DataManager::
loadRoot(Crusta* crusta, TreeIndex rootIndex, const Scope& scope)
{
    MainCache& mc = CACHE->getMainCache();

    GRAB_BUFFER(    NodeCache,     node,     mc.node, rootIndex);
    GRAB_BUFFER(GeometryCache, geometry, mc.geometry, rootIndex);
    GRAB_BUFFER(  HeightCache,   height,   mc.height, rootIndex);
    GRAB_BUFFER( ImageryCache,  imagery,  mc.imagery, rootIndex);

    nodeData.index     = rootIndex;
    nodeData.scope     = scope;
    nodeData.demTile   = hasDemData()   ? 0 : INVALID_TILEINDEX;
    nodeData.colorTile = hasColorData() ? 0 : INVALID_TILEINDEX;

    //clear the old quadtreefile tile indices
    for (int i=0; i<4; ++i)
    {
        nodeData.childDemTiles[i]   = INVALID_TILEINDEX;
        nodeData.childColorTiles[i] = INVALID_TILEINDEX;
    }

    //clear the old line data
    nodeData.lineCoverage.clear();
    nodeData.lineNumSegments = 0;
    nodeData.lineData.clear();

    sourceDem(   NULL, NULL, &nodeData,   heightData);
    sourceColor( NULL, NULL, &nodeData,  imageryData);
    generateGeometry(crusta, &nodeData, geometryData);

    nodeData.init(SETTINGS->globeRadius, crusta->getVerticalScale());

    mc.node.releaseBuffer(rootIndex, nodeBuf);
    mc.node.pin(nodeBuf);
    mc.geometry.releaseBuffer(rootIndex, geometryBuf);
    mc.geometry.pin(geometryBuf);
    mc.height.releaseBuffer(rootIndex, heightBuf);
    mc.height.pin(heightBuf);
    mc.imagery.releaseBuffer(rootIndex, imageryBuf);
    mc.imagery.pin(imageryBuf);
}


void DataManager::
frame()
{
CRUSTA_DEBUG_OUT(14, "\n++++++++  DataManager::frame() //\n");
CRUSTA_DEBUG(14, MainCache& mc=CACHE->getMainCache(); mc.node.printCache());

    //reprioritize the requests (do this here as the rest blocks fetching)
    std::sort(childRequests.begin(), childRequests.end(),
              std::greater<Request>());

    //block fetching as we manipulate the requests
    Threads::Mutex::Lock lock(requestMutex);

    //giver the cache update 0.08ms (8ms for 120 fps, so 1% of frame time)
    double endTime = CURRENT_FRAME + 0.08e-3;

    //update as much as possible
    int numFetches = SETTINGS->dataManMaxFetchRequests -
                     static_cast<int>(fetchRequests.size());
    for (Requests::iterator it=childRequests.begin();
         numFetches>0 && CURRENT_FRAME<endTime && it!=childRequests.end(); ++it)
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
        fetch.crusta = it->crusta;
        fetch.parent = it->parent;
        fetch.which  = it->child;
///\todo data needs to be grabbed more flexibly
        TreeIndex childIndex =
            it->parent.node->getData().index.down(fetch.which);
        fetch.child  = grabMainBuffer(childIndex, false);
        if (!isComplete(fetch.child))
        {
CRUSTA_DEBUG_OUT(10,"Datamanager::frame: no more room in the cache for ");
CRUSTA_DEBUG_OUT(10,"new data\n");
            break;
        }

        //submit the fetch request
        fetchRequests.push_back(fetch);
        fetchCond.signal();
        --numFetches;
    }
    childRequests.clear();

    //inject the processed requests into the tree
    for (FetchRequests::iterator it=fetchResults.begin();
         it!=fetchResults.end(); ++it)
    {
        //validate buffers
        TreeIndex childIndex = it->parent.node->getData().index.down(it->which);
        releaseMainBuffer(childIndex, it->child);

CRUSTA_DEBUG_OUT(14, "MainCache::frame: request for Index %s:%d processed\n",
childIndex.med_str().c_str(), it->which);
    }
    fetchResults.clear();

CRUSTA_DEBUG_OUT(14, "\n********  DataManager::frame() //end\n");
CRUSTA_DEBUG(14, MainCache& mc=CACHE->getMainCache(); mc.node.printCache());
}

void DataManager::
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

void DataManager::
request(const Requests& reqs)
{
    //make sure merging of the requests is done one at a time
    Threads::Mutex::Lock lock(requestMutex);

    //potential request duplication handled at fetch time, so just append here
    childRequests.insert(childRequests.end(), reqs.begin(), reqs.end());

    //request a frame to process these requests
    if (!childRequests.empty())
        Vrui::requestUpdate();
}


const NodeMainData DataManager::
getData(const NodeMainBuffer& mainBuf) const
{
    NodeMainData ret;
    ret.node     = &mainBuf.node->getData();
    ret.geometry =  mainBuf.geometry->getData();
    ret.height   =  mainBuf.height->getData();
    ret.imagery  =  mainBuf.imagery->getData();
    return ret;
}

const NodeGpuData DataManager::
getData(const NodeGpuBuffer& gpuBuf) const
{
    NodeGpuData ret;
    ret.geometry = &gpuBuf.geometry->getData();
    ret.height   = &gpuBuf.height->getData();
    ret.imagery  = &gpuBuf.imagery->getData();
    ret.coverage = &gpuBuf.coverage->getData();
    ret.lineData = &gpuBuf.lineData->getData();
    return ret;
}

bool DataManager::
existsChildData(const NodeMainData& parent)
{
    bool ret = false;
    for (int i=0; i<4; ++i)
    {
        if (parent.node->childDemTiles[i]   != INVALID_TILEINDEX ||
            parent.node->childColorTiles[i] != INVALID_TILEINDEX)
        {
            ret = true;
        }
    }
    return ret;
}

#define FIND_BUFFER(buf, cache, index)\
buf = cache.find(index);\
if (buf==NULL || !cache.isValid(buf))\
    return false

bool DataManager::
find(const TreeIndex& index, NodeMainBuffer& mainBuf) const
{
    MainCache& mc = CACHE->getMainCache();
    FIND_BUFFER(mainBuf.node,         mc.node, index);
    FIND_BUFFER(mainBuf.geometry, mc.geometry, index);
    FIND_BUFFER(mainBuf.height,     mc.height, index);
    FIND_BUFFER(mainBuf.imagery,   mc.imagery, index);

    return true;
}


void DataManager::
startGpuBatch(GLContextData& contextData, const SurfaceApproximation& surface,
              Batch& batch)
{
    //check if the GPU caches need to be cleared
    GlData::Item* glItem = contextData.retrieveDataItem<GlData::Item>(glData);
    if (glItem->clearGpuCachesStamp != clearGpuCachesStamp)
    {
        GpuCache& gc = CACHE->getGpuCache(contextData);
        gc.geometry.clear();
        gc.height.clear();
        gc.imagery.clear();
        gc.coverage.clear();
        gc.lineData.clear();

        //validate the clear
        glItem->clearGpuCachesStamp = clearGpuCachesStamp;
    }

    batch.clear();

    //go through all the render nodes and collect the appropriate data
    size_t numNodes = surface.visibles.size();
    for (batchIndex=0; batchIndex<numNodes; ++batchIndex)
    {
        //make sure the cached data is up to date
        BatchElement batchel;
        batchel.main = surface.visible(batchIndex);
        if (!streamGpuData(contextData, batchel))
            break;
        CHECK_GLA;

        //add the node and data to the current batch
        batch.push_back(batchel);
    }
}

void DataManager::
nextGpuBatch(GLContextData& contextData, const SurfaceApproximation& surface,
             Batch& batch)
{
    batch.clear();

    //are any more batches even possible
    size_t numNodes = surface.visibles.size() - batchIndex;
    if (numNodes == 0)
        return;

    //try to reset enough cache entries for the remaining nodes
    GpuCache& gpuCache = CACHE->getGpuCache(contextData);
    gpuCache.geometry.reset(numNodes);
    gpuCache.height.reset(numNodes);
    gpuCache.imagery.reset(numNodes);
    gpuCache.coverage.reset(numNodes);
    gpuCache.lineData.reset(numNodes);

    numNodes = surface.visibles.size();
    for (; batchIndex<numNodes; ++batchIndex)
    {
        //make sure the cached data is up to date
        BatchElement batchel;
        batchel.main = surface.visible(batchIndex);
        if (!streamGpuData(contextData, batchel))
            break;
        CHECK_GLA;

        //add the node and data to the current batch
        batch.push_back(batchel);
    }
}


bool DataManager::
isComplete(const NodeMainBuffer& mainBuf) const
{
    return mainBuf.node  !=NULL && mainBuf.geometry!=NULL &&
           mainBuf.height!=NULL && mainBuf.imagery !=NULL;
}

bool DataManager::
isComplete(const NodeGpuBuffer& gpuBuf) const
{
    return gpuBuf.geometry!=NULL && gpuBuf.height  !=NULL &&
           gpuBuf.imagery !=NULL && gpuBuf.coverage!=NULL &&
           gpuBuf.lineData!=NULL;
}

void DataManager::
touch(NodeMainBuffer& mainBuf) const
{
    MainCache& mc = CACHE->getMainCache();
    mc.node.touch(mainBuf.node);
    mc.geometry.touch(mainBuf.geometry);
    mc.height.touch(mainBuf.height);
    mc.imagery.touch(mainBuf.imagery);
}

void DataManager::
pin(NodeMainBuffer& mainBuf) const
{
    MainCache& mc = CACHE->getMainCache();
    mc.node.pin(mainBuf.node);
    mc.geometry.pin(mainBuf.geometry);
    mc.height.pin(mainBuf.height);
    mc.imagery.pin(mainBuf.imagery);
}

void DataManager::
unpin(NodeMainBuffer& mainBuf) const
{
    MainCache& mc = CACHE->getMainCache();
    mc.node.unpin(mainBuf.node);
    mc.geometry.unpin(mainBuf.geometry);
    mc.height.unpin(mainBuf.height);
    mc.imagery.unpin(mainBuf.imagery);
}


DataManager::FetchRequest::
FetchRequest() :
    crusta(NULL), which(~0)
{
}

bool DataManager::FetchRequest::
operator ==(const FetchRequest& other) const
{
    return parent.node->getData().index == other.parent.node->getData().index &&
           which == other.which;
}


void DataManager::GlData::
initContext(GLContextData& contextData) const
{
    Item* item = new Item;
    item->clearGpuCachesStamp = 0;
    contextData.addDataItem(this, item);
}



#define GET_BUFFER(ret, cache, index, check)\
ret = cache.find(index);\
if (ret == NULL)\
    ret = cache.grabBuffer(check)

const NodeMainBuffer DataManager::
grabMainBuffer(const TreeIndex& index, bool grabCurrent) const
{
    MainCache& mc = CACHE->getMainCache();

    NodeMainBuffer ret;
    GET_BUFFER(ret.node,         mc.node, index, grabCurrent);
    GET_BUFFER(ret.geometry, mc.geometry, index, grabCurrent);
    GET_BUFFER(ret.height,     mc.height, index, grabCurrent);
    GET_BUFFER(ret.imagery,   mc.imagery, index, grabCurrent);

    return ret;
}

void DataManager::
releaseMainBuffer(const TreeIndex& index, const NodeMainBuffer& buffer) const
{
    MainCache& mc = CACHE->getMainCache();
    if (buffer.node != NULL)
        mc.node.releaseBuffer(index, buffer.node);
    if (buffer.geometry != NULL)
        mc.geometry.releaseBuffer(index, buffer.geometry);
    if (buffer.height != NULL)
        mc.height.releaseBuffer(index, buffer.height);
    if (buffer.imagery != NULL)
        mc.imagery.releaseBuffer(index, buffer.imagery);
}


bool DataManager::
streamGpuData(GLContextData& contextData, BatchElement& batchel)
{
    GpuCache& cache        = CACHE->getGpuCache(contextData);
    NodeMainData& main     = batchel.main;
    NodeGpuData&  gpu      = batchel.gpu;
    const TreeIndex& index = main.node->index;

    CHECK_GLA;
//- handle the geometry data
    //acquire buffer
    GpuGeometryCache::BufferType* geometry = cache.geometry.find(index);
    if (geometry == NULL)
    {
        geometry = cache.geometry.grabBuffer(false);
        if (geometry == NULL)
            return false;
    }
    //update data
    gpu.geometry = &geometry->getData();
    if (cache.geometry.isGrabbed(geometry) || !cache.geometry.isValid(geometry))
    {
        cache.geometry.stream(*gpu.geometry, GL_RGB, GL_FLOAT, main.geometry);
        CHECK_GLA;
    }
    //validate buffer
    cache.geometry.releaseBuffer(index, geometry);

//- handle the height data
    //acquire buffer
    GpuHeightCache::BufferType* height = cache.height.find(index);
    if (height == NULL)
    {
        height = cache.height.grabBuffer(false);
        if (height == NULL)
            return false;
    }
    //update data
    gpu.height = &height->getData();
    if (cache.height.isGrabbed(height) || !cache.height.isValid(height))
    {
        cache.height.stream(*gpu.height, GL_RED, GL_FLOAT, main.height);
        CHECK_GLA;
    }
    //validate buffer
    cache.height.releaseBuffer(index, height);

//- handle the imagery data
    //acquire buffer
    GpuImageryCache::BufferType* imagery = cache.imagery.find(index);
    if (imagery == NULL)
    {
        imagery = cache.imagery.grabBuffer(false);
        if (imagery == NULL)
            return false;
    }
    //update data
    gpu.imagery = &imagery->getData();
    if (cache.imagery.isGrabbed(imagery) || !cache.imagery.isValid(imagery))
    {
        cache.imagery.stream(*gpu.imagery,GL_RGB,GL_UNSIGNED_BYTE,main.imagery);
        CHECK_GLA;
    }
    //validate buffer
    cache.imagery.releaseBuffer(index, imagery);

    //decorated vector art requires up-to-date line data and coverage textures
    if (SETTINGS->decorateVectorArt && main.node->lineNumSegments!=0)
    {
    //- handle the line data
        //acquire buffer
        GpuLineDataCache::BufferType* lineData = cache.lineData.find(index);
        if (lineData == NULL)
        {
            lineData = cache.lineData.grabBuffer(false);
            if (lineData == NULL)
                return false;
        }
        //update data
        gpu.lineData = &lineData->getData();
        bool updatedLine = false;
        if (cache.lineData.isGrabbed(lineData) ||
            !cache.lineData.isValid(lineData)  ||
            gpu.lineData->age < main.node->lineCoverageAge)
        {
            //stream the line data from the main representation
            cache.lineData.stream((SubRegion)(*gpu.lineData), GL_RGBA,
                                  GL_FLOAT, &main.node->lineData.front());
            CHECK_GLA;
            //stamp the age of the new data
            gpu.lineData->age = CURRENT_FRAME;
            updatedLine = true;
        }
        //validate buffer
        cache.lineData.releaseBuffer(index, lineData);

    //- handle the coverage data
        //acquire buffer
        GpuCoverageCache::BufferType* coverage = cache.coverage.find(index);
        if (coverage == NULL)
        {
            coverage = cache.coverage.grabBuffer(false);
            if (coverage == NULL)
                return false;
        }
        //update the data
        gpu.coverage = &coverage->getData();
        if (updatedLine || cache.coverage.isGrabbed(coverage) ||
            !cache.coverage.isValid(coverage))
        {
            //render the new coverage into the coverage texture
            CHECK_GLA;
            cache.coverage.beginRender(*gpu.coverage);
            CHECK_GLA;
            QuadTerrain::renderLineCoverageMap(contextData, main);
            CHECK_GLA;
            cache.coverage.endRender();
            CHECK_GLA;
        }
        //validate buffer
        cache.coverage.releaseBuffer(index, coverage);
    }

    return true;
}

void DataManager::
loadChild(Crusta* crusta, NodeMainData& parent,
          uint8 which,    NodeMainData& child)
{
    //compute the child scopes
    Scope childScopes[4];
    parent.node->scope.split(childScopes);

    child.node->index     = parent.node->index.down(which);
    child.node->scope     = childScopes[which];
    child.node->demTile   = parent.node->childDemTiles[which];
    child.node->colorTile = parent.node->childColorTiles[which];

    //clear the old quadtreefile tile indices
    for (int i=0; i<4; ++i)
    {
        child.node->childDemTiles[i]   = INVALID_TILEINDEX;
        child.node->childColorTiles[i] = INVALID_TILEINDEX;
    }

    //clear the old line data
    child.node->lineCoverage.clear();
    child.node->lineNumSegments = 0;
    child.node->lineData.clear();

    //grab the proper data for the child
    sourceDem(parent.node, parent.height, child.node, child.height);
    sourceColor(parent.node, parent.imagery, child.node, child.imagery);
    generateGeometry(crusta, child.node, child.geometry);

    child.node->init(SETTINGS->globeRadius, crusta->getVerticalScale());

/**\todo Vis2010 This is where the coverage data should be propagated to the
child. But, here I only see individual children, thus I'd have to split the
dirty bit to reflect that individuality. For now, since even loaded nodes need
to be inserted proper after all checks pass in QuadTerrain, just leave it up to
that to pass along the proper data */
}


void DataManager::
generateGeometry(Crusta* crusta, NodeData* child, Vertex* v)
{
///\todo use average height to offset from the spheroid
    double shellRadius = SETTINGS->globeRadius;
    child->scope.getRefinement(shellRadius, TILE_RESOLUTION, tempGeometryBuf);

    /* compute and store the centroid here, since node-creation level generation
     of these values only happens after the data load step */
    Scope::Vertex scopeCentroid = child->scope.getCentroid(shellRadius);
    child->centroid[0] = scopeCentroid[0];
    child->centroid[1] = scopeCentroid[1];
    child->centroid[2] = scopeCentroid[2];

    for (double* g=tempGeometryBuf;
         g<tempGeometryBuf+TILE_RESOLUTION*TILE_RESOLUTION*3; g+=3, ++v)
    {
        v->position[0] = DemHeight(g[0] - child->centroid[0]);
        v->position[1] = DemHeight(g[1] - child->centroid[1]);
        v->position[2] = DemHeight(g[2] - child->centroid[2]);
    }
}

template <typename PixelParam>
inline void
sampleParentBase(int child, PixelParam range[2], PixelParam* dst,
                 const PixelParam* const src, const PixelParam& nodata)
{
    typedef PixelOps<PixelParam> po;

    static const int offsets[4] = {
        0, (TILE_RESOLUTION-1)>>1, ((TILE_RESOLUTION-1)>>1)*TILE_RESOLUTION,
        ((TILE_RESOLUTION-1)>>1)*TILE_RESOLUTION + ((TILE_RESOLUTION-1)>>1) };
    int halfSize[2] = { (TILE_RESOLUTION+1)>>1, (TILE_RESOLUTION+1)>>1 };

    for (int y=0; y<halfSize[1]; ++y)
    {
        PixelParam*       wbase = dst + y*2*TILE_RESOLUTION;
        const PixelParam* rbase = src + y*TILE_RESOLUTION + offsets[child];

        for (int x=0; x<halfSize[0]; ++x, wbase+=2, ++rbase)
        {
            range[0] = po::minimum(range[0], rbase[0], nodata);
            range[1] = po::maximum(range[1], rbase[0], nodata);

            wbase[0] = rbase[0];
            if (x<halfSize[0]-1)
                wbase[1] = po::average(rbase[0], rbase[1], nodata);
            if (y<halfSize[1]-1)
            {
                wbase[TILE_RESOLUTION] =
                    po::average(rbase[0], rbase[TILE_RESOLUTION], nodata);
            }
            if (x<halfSize[0]-1 && y<halfSize[1]-1)
            {
                wbase[TILE_RESOLUTION+1] = po::average(rbase[0], rbase[1],
                    rbase[TILE_RESOLUTION], rbase[TILE_RESOLUTION+1], nodata);
            }
        }
    }
}

inline void
sampleParent(int child, DemHeight range[2], DemHeight* dst,
             const DemHeight* const src, const DemHeight& nodata)
{
    range[0] = Math::Constants<DemHeight>::max;
    range[1] = Math::Constants<DemHeight>::min;

    sampleParentBase(child, range, dst, src, nodata);
}

inline void
sampleParent(int child, TextureColor range[2], TextureColor* dst,
             const TextureColor* const src, const TextureColor& nodata)
{
    range[0] = TextureColor(255,255,255);
    range[1] = TextureColor(0,0,0);

    sampleParentBase(child, range, dst, src, nodata);
}

void DataManager::
sourceDem(const NodeData* const parent, const DemHeight* const parentHeight,
          NodeData* child, DemHeight* childHeight)
{
    typedef DemFile::File    File;
    typedef File::TileHeader TileHeader;

    DemHeight* range   = &child->elevationRange[0];

    if (child->demTile != INVALID_TILEINDEX)
    {
        TileHeader header;
        File* file = demFile->getPatch(child->index.patch);
        if (!file->readTile(child->demTile, child->childDemTiles, header,
                            childHeight))
        {
            Misc::throwStdErr("DataManager::sourceDem: Invalid DEM file: "
                              "could not read node %s's data",
                              child->index.med_str().c_str());
        }
        range[0] = header.range[0];
        range[1] = header.range[1];
    }
    else
    {
        if (parent != NULL)
            sampleParent(child->index.child, range, childHeight, parentHeight,
                         demNodata);
        else
        {
            range[0] =  Math::Constants<DemHeight>::max;
            range[1] = -Math::Constants<DemHeight>::max;
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                childHeight[i] = demNodata;
        }
    }
}

void DataManager::
sourceColor(
    const NodeData* const parent, const TextureColor* const parentImagery,
    NodeData* child, TextureColor* childImagery)
{
    typedef ColorFile::File File;

    if (child->colorTile != INVALID_TILEINDEX)
    {
        File* file = colorFile->getPatch(child->index.patch);
        if (!file->readTile(child->colorTile, child->childColorTiles,
                            childImagery))
        {
            Misc::throwStdErr("DataManager::sourceColor: Invalid Color "
                              "file: could not read node %s's data",
                              child->index.med_str().c_str());
        }
    }
    else
    {
        TextureColor range[2];
        if (parent != NULL)
            sampleParent(child->index.child, range, childImagery, parentImagery,
                         colorNodata);
        else
        {
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                childImagery[i] = colorNodata;
        }
    }
}

void* DataManager::
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
        NodeMainData parentData = getData(fetch->parent);
        NodeMainData childData  = getData(fetch->child);
        loadChild(fetch->crusta, parentData, fetch->which, childData);
    }

    return NULL;
}


DataManager* DATAMANAGER;


END_CRUSTA
