#include <crusta/DataManager.h>

#include <sstream>

#include <Math/Constants.h>

#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>
#include <crusta/Polyhedron.h>
#include <crusta/QuadCache.h>
#include <crusta/QuadTerrain.h>


BEGIN_CRUSTA


DataManager::NodeMainBuffer::
NodeMainBuffer() :
    node(NULL), geometry(NULL), height(NULL), imagery(NULL)
{
}

DataManager::NodeMainData::
NodeMainData() :
    node(NULL), geometry(NULL), height(NULL), imagery(NULL)
{
}


DataManager::NodeGpuBuffer::
NodeGpuBuffer() :
    geometry(NULL), height(NULL), imagery(NULL), coverage(NULL), lineData(NULL)
{
}

DataManager::NodeGpuData::
NodeGpuData() :
    geometry(NULL), height(NULL), imagery(NULL), coverage(NULL), lineData(NULL)
{
}


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
DataManager(const Polyhedron& polyhedron, const std::string& demBase,
            const std::string& colorBase)
{
    uint resolution[2] = { TILE_RESOLUTION, TILE_RESOLUTION };
    uint numPatches = polyhedron.getNumPatches();

    if (!demBase.empty())
    {
        demFiles.resize(numPatches);
        for (uint i=0; i<numPatches; ++i)
        {
            std::ostringstream demName;
            demName << demBase << "_" << i << ".qtf";
            demFiles[i] = new DemFile(demName.str().c_str(), resolution);
        }
        demNodata = demFiles[0]->getDefaultPixelValue();
    }
    else
        demNodata = DemHeight(0);

    if (!colorBase.empty())
    {
        colorFiles.resize(numPatches);
        for (uint i=0; i<numPatches; ++i)
        {
            std::ostringstream colorName;
            colorName << colorBase << "_" << i << ".qtf";
            colorFiles[i] = new ColorFile(colorName.str().c_str(), resolution);
        }
        colorNodata = colorFiles[0]->getDefaultPixelValue();
    }
    else
        colorNodata = TextureColor(128, 128, 128);

    tempGeometryBuf = new double[TILE_RESOLUTION*TILE_RESOLUTION*3];

    //start the fetching thread
    fetchThread.start(this, &DataManager::fetchThreadFunc);
}

DataManager::
~DataManager()
{
    for (DemFiles::iterator it=demFiles.begin(); it!=demFiles.end(); ++it)
        delete *it;
    for (ColorFiles::iterator it=colorFiles.begin(); it!=colorFiles.end(); ++it)
        delete *it;

    delete[] tempGeometryBuf;

    //stop the fetching thread
    fetchThread.cancel();
    fetchThread.join();
}


bool DataManager::
hasDemData() const
{
    return !demFiles.empty();
}

bool DataManager::
hasColorData() const
{
    return !colorFiles.empty();
}


#define GRAB_BUFFER(cacheType, name, cache, index)\
cacheType::BufferType* name##Buf = cache.find(index);\
if (name##Buf == NULL)\
    name##Buf = cache.getBuffer(index, false);\
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
    nodeData.demTile   = hasDemData()   ? 0 :   DemFile::INVALID_TILEINDEX;
    nodeData.colorTile = hasColorData() ? 0 : ColorFile::INVALID_TILEINDEX;

    sourceDem(   NULL, NULL, &nodeData,   heightData);
    sourceColor( NULL, NULL, &nodeData,  imageryData);
    generateGeometry(crusta, &nodeData, geometryData);

    nodeData.init(SETTINGS->globeRadius, crusta->getVerticalScale());

    mc.node.touch(nodeBuf);
    mc.node.pin(nodeBuf);
    mc.geometry.touch(geometryBuf);
    mc.geometry.pin(geometryBuf);
    mc.height.touch(heightBuf);
    mc.height.pin(heightBuf);
    mc.imagery.touch(imageryBuf);
    mc.imagery.pin(imageryBuf);
}


void DataManager::
frame()
{
    //reprioritize the requests (do this here as the rest blocks fetching)
    std::sort(childRequests.begin(), childRequests.end(),
              std::greater<Request>());

    //block fetching as we manipulate the requests
    Threads::Mutex::Lock lock(requestMutex);

    //giver the cache update 0.08ms (8ms for 120 fps, so 1% of frame time)
    double endTime = Vrui::getApplicationTime() + 0.08e-3;

    //update as much as possible
    int numFetches = SETTINGS->dataManMaxFetchRequests -
                     static_cast<int>(fetchRequests.size());
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
        fetch.crusta = it->crusta;
        fetch.parent = it->parent;
        fetch.which  = it->child;
        TreeIndex childIndex = fetch.parent.node->getData().index.down(
            fetch.which);
        fetch.child = getMainBuffer(childIndex);
        if (!isComplete(fetch.child))
        {
CRUSTA_DEBUG_OUT(2,"MainCache::frame: no more room in the cache for ");
CRUSTA_DEBUG_OUT(2,"new data\n");
            childRequests.clear();
            return;
        }
        //pin the buffers we obtained
        pin(fetch.parent);
        pin(fetch.child);

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
        unpin(it->parent);
        touch(it->parent);
        unpin(it->child);
        touch(it->child);

CRUSTA_DEBUG_OUT(1, "MainCache::frame: request for Index %s:%d processed\n",
it->parent.node->getData().index.med_str().c_str(), it->which);
    }
    fetchResults.clear();
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


const DataManager::NodeMainData DataManager::
getData(const NodeMainBuffer& mainBuf) const
{
    NodeMainData ret;
    ret.node     = &mainBuf.node->getData();
    ret.geometry =  mainBuf.geometry->getData();
    ret.height   =  mainBuf.height->getData();
    ret.imagery  =  mainBuf.imagery->getData();
    return ret;
}

const DataManager::NodeGpuData DataManager::
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
        if (parent.node->childDemTiles[i]  !=  DemFile::INVALID_TILEINDEX ||
            parent.node->childColorTiles[i]!=ColorFile::INVALID_TILEINDEX)
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
startGpuBatch(GLContextData& contextData, const NodeMainDatas& renderNodes,
              NodeMainDatas& batchNodes, NodeGpuDatas& batchDatas)
{
    GpuCache& gpuCache = CACHE->getGpuCache(contextData);

    remainingRenderNodes = renderNodes;
    batchNodes.clear();
    batchDatas.clear();

    //go through all the render nodes and collect the appropriate data
    NodeMainDatas::iterator it = remainingRenderNodes.begin();
    for (; it!=remainingRenderNodes.end(); ++it)
    {
        //make sure the gpu cache can support an additional node
        NodeGpuBuffer gpuBuf;
        if (!getGpuBuffer(gpuCache, it->node->index, *it, gpuBuf))
            break;

        //make sure the cached data is up to date
        streamGpuData(contextData, gpuCache, *it, gpuBuf);
        CHECK_GLA;

        //add the node and data to the current batch
        batchNodes.push_back(*it);
        batchDatas.push_back(getData(gpuBuf));
    }

    //remove all the nodes that made it to the current batch
    remainingRenderNodes.erase(remainingRenderNodes.begin(), it);
}

void DataManager::
nextGpuBatch(GLContextData& contextData, NodeMainDatas& batchNodes,
             NodeGpuDatas& batchDatas)
{
    GpuCache& gpuCache = CACHE->getGpuCache(contextData);

    batchNodes.clear();
    batchDatas.clear();

    //are any more batches even possible
    int numNodes = static_cast<int>(remainingRenderNodes.size());
    if (numNodes == 0)
        return;

    //try to reset enough cache entries for the remaining nodes
    gpuCache.geometry.reset(numNodes);
    gpuCache.height.reset(numNodes);
    gpuCache.imagery.reset(numNodes);
    gpuCache.coverage.reset(numNodes);
    gpuCache.lineData.reset(numNodes);

    NodeMainDatas::iterator it = remainingRenderNodes.begin();
    for (; it!=remainingRenderNodes.end(); ++it)
    {
        //make sure the gpu cache can support an additional node
        NodeGpuBuffer gpuBuf;
        if (!getGpuBuffer(gpuCache, it->node->index, *it, gpuBuf))
            break;

        //make sure the cached data is up to date
        streamGpuData(contextData, gpuCache, *it, gpuBuf);

        //add the node and data to the current batch
        batchNodes.push_back(*it);
        batchDatas.push_back(getData(gpuBuf));
    }

    //remove all the nodes that made it to the current batch
    remainingRenderNodes.erase(remainingRenderNodes.begin(), it);
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

#if 0
bool DataManager::FetchRequest::
operator <(const FetchRequest& other) const
{
    return (size_t)(parent) < (size_t)(other.parent);
}
#endif



#define GET_BUFFER(ret, cache, index, check)\
ret = cache.find(index);\
if (ret == NULL)\
    ret = cache.getBuffer(index, check)

const DataManager::NodeMainBuffer DataManager::
getMainBuffer(const TreeIndex& index, bool checkCurrent) const
{
    MainCache& mc = CACHE->getMainCache();

    NodeMainBuffer ret;
    GET_BUFFER(ret.node,         mc.node, index, checkCurrent);
    GET_BUFFER(ret.geometry, mc.geometry, index, checkCurrent);
    GET_BUFFER(ret.height,     mc.height, index, checkCurrent);
    GET_BUFFER(ret.imagery,   mc.imagery, index, checkCurrent);

    return ret;
}

bool DataManager::
getGpuBuffer(GpuCache& gc, const TreeIndex& index,
             const NodeMainData& main, NodeGpuBuffer& buf) const
{
    buf.geometry = gc.geometry.find(index);
    if (buf.geometry == NULL)
    {
        buf.geometry = gc.geometry.getBuffer(index);
        if (buf.geometry == NULL)
            return false;
    }
    buf.height = gc.height.find(index);
    if (buf.height == NULL)
    {
        buf.height = gc.height.getBuffer(index);
        if (buf.height == NULL)
            return false;
    }
    buf.imagery = gc.imagery.find(index);
    if (buf.imagery == NULL)
    {
        buf.imagery = gc.imagery.getBuffer(index);
        if (buf.imagery == NULL)
            return false;
    }

    if (SETTINGS->decorateVectorArt && main.node->lineNumSegments!=0)
    {
        buf.coverage = gc.coverage.find(index);
        if (buf.coverage == NULL)
        {
            buf.coverage = gc.coverage.getBuffer(index);
            if (buf.coverage == NULL)
                return false;
        }
        buf.lineData = gc.lineData.find(index);
        if (buf.lineData == NULL)
        {
            buf.lineData = gc.lineData.getBuffer(index);
            if (buf.lineData == NULL)
                return false;
        }
    }

    return true;
}


void DataManager::
streamGpuData(GLContextData& contextData, GpuCache& cache,
              NodeMainData& main, NodeGpuBuffer& gpu)
{
    CHECK_GLA;
    if (!cache.geometry.isValid(gpu.geometry))
    {
        cache.geometry.stream(gpu.geometry->getData(), GL_RGB, GL_FLOAT,
                              main.geometry);
    }
    cache.geometry.touch(gpu.geometry);
    CHECK_GLA;
    if (!cache.height.isValid(gpu.height))
    {
        cache.height.stream(gpu.height->getData(), GL_RED, GL_FLOAT,
                            main.height);
    }
    cache.height.touch(gpu.height);
    CHECK_GLA;
    if (!cache.imagery.isValid(gpu.imagery))
    {
        cache.imagery.stream(gpu.imagery->getData(), GL_RGB, GL_UNSIGNED_BYTE,
                            main.imagery);
    }
    cache.imagery.touch(gpu.imagery);
    CHECK_GLA;

    //decorated vector art requires up-to-date line data and coverage textures
    if (SETTINGS->decorateVectorArt && main.node->lineNumSegments!=0)
    {
        bool updatedLine = false;
        if (!cache.lineData.isValid(gpu.lineData) ||
            gpu.lineData->getData().age != main.node->lineCoverageAge)
        {
            //stream the line data from the main representation
            cache.lineData.stream((SubRegion)(gpu.lineData->getData()), GL_RGBA,
                                  GL_FLOAT, &main.node->lineData.front());
            cache.lineData.touch(gpu.lineData);
            CHECK_GLA;
            //stamp the age of the new data
            main.node->lineCoverageAge = Vrui::getApplicationTime();
            updatedLine = true;
        }
        if (updatedLine || !cache.coverage.isValid(gpu.coverage))
        {
            //render the new coverage into the coverage texture
            CHECK_GLA;
            cache.coverage.beginRender(gpu.coverage->getData());
            CHECK_GLA;
            QuadTerrain::renderLineCoverageMap(contextData, main);
            CHECK_GLA;
            cache.coverage.endRender();
            CHECK_GLA;
            cache.coverage.touch(gpu.coverage);
        }
    }
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
        child.node->childDemTiles[i]   =   DemFile::INVALID_TILEINDEX;
        child.node->childColorTiles[i] = ColorFile::INVALID_TILEINDEX;
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
                 const PixelParam* const src)
{
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
            range[0] = pixelMin(range[0], rbase[0]);
            range[1] = pixelMax(range[1], rbase[0]);

            wbase[0] = rbase[0];
            if (x<halfSize[0]-1)
                wbase[1] = pixelAvg(rbase[0], rbase[1]);
            if (y<halfSize[1]-1)
            {
                wbase[TILE_RESOLUTION] = pixelAvg(rbase[0],
                                                  rbase[TILE_RESOLUTION]);
            }
            if (x<halfSize[0]-1 && y<halfSize[1]-1)
            {
                wbase[TILE_RESOLUTION+1] = pixelAvg(rbase[0], rbase[1],
                                                    rbase[TILE_RESOLUTION],
                                                    rbase[TILE_RESOLUTION+1]);
            }
        }
    }
}

inline void
sampleParent(int child, DemHeight range[2], DemHeight* dst,
             const DemHeight* const src)
{
    range[0] = Math::Constants<DemHeight>::max;
    range[1] = Math::Constants<DemHeight>::min;

    sampleParentBase(child, range, dst, src);
}

inline void
sampleParent(int child, TextureColor range[2], TextureColor* dst,
             const TextureColor* const src)
{
    range[0] = TextureColor(255,255,255);
    range[1] = TextureColor(0,0,0);

    sampleParentBase(child, range, dst, src);
}

void DataManager::
sourceDem(const NodeData* const parent, const DemHeight* const parentHeight,
          NodeData* child, DemHeight* childHeight)
{
    DemHeight* range = &child->elevationRange[0];

    if (child->demTile != DemFile::INVALID_TILEINDEX)
    {
        DemTileHeader header;
        DemFile* file = demFiles[child->index.patch];
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
            sampleParent(child->index.child, range, childHeight, parentHeight);
        else
        {
            range[0] = range[1] = demNodata;
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
    if (child->colorTile != ColorFile::INVALID_TILEINDEX)
    {
        ColorFile* file = colorFiles[child->index.patch];
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
            sampleParent(child->index.child, range, childImagery,parentImagery);
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
