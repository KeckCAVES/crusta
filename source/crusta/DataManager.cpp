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


DataManager::SourceShaders::
SourceShaders(int numColorLayers, int numLayerfLayers) :
    geometry("geometryTex"), height("layerfTex"),
    coverage("lineCoverageTex"), lineData("lineDataTex"),
    topography(&geometry, &height)
{
    assert(colors.empty());
    colors.reserve(numColorLayers);
    for (int i=0; i<numColorLayers; ++i)
        colors.push_back(Shader2dAtlasDataSource("colorTex"));

    assert(layers.empty());
    layers.reserve(numLayerfLayers);
    for (int i=0; i<numLayerfLayers; ++i)
        layers.push_back(Shader2dAtlasDataSource("layerfTex"));
}

void DataManager::SourceShaders::
reset()
{
    geometry.reset();
    height.reset();
    coverage.reset();
    lineData.reset();
    topography.reset();

    typedef Shader2dAtlasDataSources::iterator Iterator;
    for (Iterator it=colors.begin(); it!=colors.end(); ++it)
        it->reset();
    for (Iterator it=layers.begin(); it!=layers.end(); ++it)
        it->reset();
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
    /* since everything is fetched on a node basis the tree index and the child
       identifier is all that is needed to coalesce requests */
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
    demFile(NULL), polyhedron(NULL),
    demNodata(GlobeData<DemHeight>::defaultNodata()),
    colorNodata(GlobeData<TextureColor>::defaultNodata()),
    layerfNodata(GlobeData<LayerDataf>::defaultNodata()),
    terminateFetch(false), resetSourceShadersStamp(0)
{
    tempGeometryBuf = new double[TILE_RESOLUTION*TILE_RESOLUTION*3];
}

DataManager::
~DataManager()
{
    unload();

    delete tempGeometryBuf;
}

void DataManager::
load(Strings& dataPaths)
{
    //detach from existing databases
    unload();

/**\todo check for mismatching polyhedra and no-data values
for now just use the default polyhedron and no-data irrespective of sources */

    /* go through the paths and try to load corresponding data sources. Remove
       unsupported paths to communicate back what was actually loaded */
    for (Strings::iterator it=dataPaths.begin(); it!=dataPaths.end(); )
    {
    //- load DEM?
        if (DemFile::isCompatible(*it))
        {
            //only a single DEM is supported so check we haven't one already
            if (demFile == NULL)
            {
                demFile = new DemFile;
                try
                {
                    demFile->open(*it);
                    demFilePath = *it;
                    while (demFilePath[demFilePath.size()-1]=='/')
                        demFilePath.resize(demFilePath.size()-1);
                    ++it;
                }
                catch (std::runtime_error e)
                {
                    delete demFile;
                    demFile = NULL;
                    it = dataPaths.erase(it);

                    std::cerr << e.what();
                }
            }
        }
    //- load Color?
        else if (ColorFile::isCompatible(*it))
        {
            ColorFile* file = new ColorFile;
            try
            {
                file->open(*it);
                colorFiles.push_back(file);
                colorFilePaths.push_back(*it);
                std::string& path = colorFilePaths.back();
                while (path[path.size()-1]=='/')
                    path.resize(path.size()-1);
                ++it;
            }
            catch (std::runtime_error e)
            {
                delete file;
                it = dataPaths.erase(it);

                std::cerr << e.what();
            }
        }
    //- load Layerf?
        else if (LayerfFile::isCompatible(*it))
        {
            LayerfFile* file = new LayerfFile;
            try
            {
                file->open(*it);
                layerfFiles.push_back(file);
                layerfFilePaths.push_back(*it);
                std::string& path = layerfFilePaths.back();
                while (path[path.size()-1]=='/')
                    path.resize(path.size()-1);
                ++it;
            }
            catch (std::runtime_error e)
            {
                delete file;
                it = dataPaths.erase(it);

                std::cerr << e.what();
            }
        }
    //- not recognized data base
        else
        {
            it = dataPaths.erase(it);
        }
    }

///\todo get the no-data values from the files
    demNodata    = GlobeData<DemHeight>::defaultNodata();
    colorNodata  = GlobeData<TextureColor>::defaultNodata();
    layerfNodata = GlobeData<LayerDataf>::defaultNodata();

///\todo get the polyhedron from the files and check compatibility
    polyhedron = new Triacontahedron(SETTINGS->globeRadius);

    //reset the data source shaders
    resetSourceShadersStamp = CURRENT_FRAME;

    startFetchThread();
}

void DataManager::
unload()
{
    //stop the fetching thread
    terminateFetchThread();

    //clear all the requests
    childRequests.clear();
    fetchRequests.clear();
    fetchResults.clear();

    //clear the main memory caches and flag the GPU ones
    CACHE->clear();

    //delete the open data files
    if (demFile)
    {
        delete demFile;
        demFile = NULL;
        demFilePath.clear();
    }

    for (ColorFiles::iterator it=colorFiles.begin(); it!=colorFiles.end(); ++it)
    {
        delete *it;
    }
    colorFiles.clear();
    colorFilePaths.clear();

    for (LayerfFiles::iterator it=layerfFiles.begin(); it!=layerfFiles.end();
         ++it)
    {
        delete *it;
    }
    layerfFiles.clear();
    layerfFilePaths.clear();

    //get rid of the polyhedron
    if (polyhedron)
    {
        delete polyhedron;
        polyhedron = NULL;
    }
}


bool DataManager::
hasDem() const
{
    return demFile!=NULL;
}

const Polyhedron* const DataManager::
getPolyhedron() const
{
    return polyhedron;
}

const DemHeight::Type& DataManager::
getDemNodata()
{
    return demNodata;
}

const TextureColor::Type& DataManager::
getColorNodata()
{
    return colorNodata;
}

const LayerDataf::Type& DataManager::
getLayerfNodata()
{
    return layerfNodata;
}


const std::string& DataManager::
getDemFilePath() const
{
    return demFilePath;
}

const DataManager::Strings& DataManager::
getColorFilePaths() const
{
    return colorFilePaths;
}

const DataManager::Strings& DataManager::
getLayerfFilePaths() const
{
    return layerfFilePaths;
}

const int DataManager::
getNumColorLayers() const
{
    return static_cast<int>(colorFiles.size());
}

const int DataManager::
getNumLayerfLayers() const
{
    return static_cast<int>(layerfFiles.size());
}

LayerDataf::Type DataManager::
sampleLayerf(int which, const SurfacePoint& point)
{
    DataIndex index(which, point.nodeIndex);

    MainCache& mc = CACHE->getMainCache();
    LayerfCache::BufferType* buf = mc.layerf.find(index);
    assert(buf != NULL);

    //sample the data cell
    int linearOffset = point.cellIndex[1]*TILE_RESOLUTION + point.cellIndex[0];
    LayerDataf::Type* cell = buf->getData() + linearOffset;

///\todo use the Filter classes for this (see SubsampleFilter)
    const LayerDataf::Type corners[4] = {
        cell[0], cell[1], cell[TILE_RESOLUTION], cell[TILE_RESOLUTION+1]
    };
    double weights[4] = {
    (1-point.cellPosition[1]) * (1-point.cellPosition[0]),
    (1-point.cellPosition[1]) * point.cellPosition[0],
        point.cellPosition[1] * (1-point.cellPosition[0]),
        point.cellPosition[1] * point.cellPosition[0] };

    double sum        = 0.0;
    double sumWeights = 0.0;
    for (int i=0; i<4; ++i)
    {
        if (corners[i] != layerfNodata)
        {
            sum        += corners[i] * weights[i];
            sumWeights += weights[i];
        }
    }

    if (sumWeights == 0.0)
        return layerfNodata;

    return LayerDataf::Type(sum / sumWeights);
}

DataManager::SourceShaders& DataManager::
getSourceShaders(GLContextData& contextData)
{
    GlItem* glItem = contextData.retrieveDataItem<GlItem>(this);
    return *glItem->sourceShaders;
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
cacheType::BufferType::DataType& name##Data = name##Buf->getData();

#define RELEASE_PIN_BUFFER(cache, index, buffer)\
cache.releaseBuffer(index, buffer);\
cache.pin(buffer);

void DataManager::
loadRoot(Crusta* crusta, TreeIndex rootIndex, const Scope& scope)
{
    MainCache& mc = CACHE->getMainCache();

    int numColorLayers = static_cast<int>(colorFiles.size());
    int numFloatLayers = static_cast<int>(layerfFiles.size());

//- Node data
    DataIndex rootDataIndex(0, rootIndex);
    GRAB_BUFFER(NodeCache, node, mc.node, rootDataIndex)

    //clear the data layers
    nodeData.colorTiles.resize(numColorLayers);
    nodeData.layerTiles.resize(numFloatLayers);

    //clear the old line data
    nodeData.lineCoverage.clear();
    nodeData.lineNumSegments = 0;
    nodeData.lineData.clear();

    //initialize
    nodeData.index = rootIndex;
    nodeData.scope = scope;

//- Geometry data
    {
        DataIndex index(0, rootIndex);
        GRAB_BUFFER(GeometryCache, geometry, mc.geometry, index)
        generateGeometry(crusta, &nodeData, geometryData);
        RELEASE_PIN_BUFFER(mc.geometry, index, geometryBuf)
    }

//- Topography data
    {
        nodeData.demTile.node = hasDem() ? 0 : INVALID_TILEINDEX;
        for (int i=0; i<4; ++i)
            nodeData.demTile.children[i] = INVALID_TILEINDEX;

        //topography reserves the first data id of the layerf cache
        DataIndex index(0, rootIndex);
        GRAB_BUFFER(LayerfCache, height, mc.layerf, index)
        sourceDem(NULL, NULL, &nodeData, heightData);
        RELEASE_PIN_BUFFER(mc.layerf, index, heightBuf)
    }

//- Texture color layer data
    for (int l=0; l<numColorLayers; ++l)
    {
        //generate and save the tile indices for this data
        NodeData::Tile& tile = nodeData.colorTiles[l];
        tile.dataId = l;
        tile.node   = 0;
        for (int c=0; c<4; ++c)
            tile.children[c] = INVALID_TILEINDEX;

        //read in the color data
        DataIndex index(l, rootIndex);
        GRAB_BUFFER(ColorCache, layer, mc.color, index)
        sourceColor(NULL, NULL, &nodeData, l, layerData);
        RELEASE_PIN_BUFFER(mc.color, index, layerBuf)
    }

    //topography reserved the 0th layer of the layerf cache
//- Layerf layer data
    for (int l=0; l<numFloatLayers; ++l)
    {
        //generate and save the tile indices for this data
        NodeData::Tile& tile = nodeData.layerTiles[l];
        tile.dataId = l+1;
        tile.node   = 0;
        for (int c=0; c<4; ++c)
            tile.children[c] = INVALID_TILEINDEX;

        //read in the layer
        DataIndex index(l+1, rootIndex);
        GRAB_BUFFER(LayerfCache, layer, mc.layerf, index)
        sourceLayerf(NULL, NULL, &nodeData, l, layerData);
        RELEASE_PIN_BUFFER(mc.layerf, index, layerBuf)
    }

//- Finalize the node
    nodeData.init(SETTINGS->globeRadius, crusta->getVerticalScale());
    RELEASE_PIN_BUFFER(mc.node, rootDataIndex, nodeBuf)
}


void DataManager::
frame()
{
CRUSTA_DEBUG(14, CRUSTA_DEBUG_OUT << "\n++++++++  DataManager::frame() //\n";
             MainCache& mc=CACHE->getMainCache(); mc.node.printCache();)

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
CRUSTA_DEBUG(10, CRUSTA_DEBUG_OUT <<
"Datamanager::frame: no more room in the cache for new data\n";)
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

CRUSTA_DEBUG(14, CRUSTA_DEBUG_OUT <<
"MainCache::frame: request for Index " << childIndex.med_str() << ":" <<
it->which << " processed\n";)
    }
    fetchResults.clear();

CRUSTA_DEBUG(14, CRUSTA_DEBUG_OUT <<
"\n********  DataManager::frame() //end\n"; MainCache& mc=CACHE->getMainCache();
mc.node.printCache();)
}

void DataManager::
display(GLContextData& contextData)
{
    //check if the GPU caches need to be cleared
    GlItem* glItem = contextData.retrieveDataItem<GlItem>(this);
    if (glItem->resetSourceShadersStamp < resetSourceShadersStamp)
    {
        delete glItem->sourceShaders;
        glItem->sourceShaders = new SourceShaders( colorFiles.size(),
                                                  layerfFiles.size());
        glItem->resetSourceShadersStamp = CURRENT_FRAME;
    }
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

    typedef NodeMainBuffer::ColorBufferPtrs::const_iterator ColorIte;
    for (ColorIte it=mainBuf.colors.begin(); it!=mainBuf.colors.end(); ++it)
        ret.colors.push_back((*it)->getData());

    typedef NodeMainBuffer::LayerBufferPtrs::const_iterator LayerIte;
    for (LayerIte it=mainBuf.layers.begin(); it!=mainBuf.layers.end(); ++it)
        ret.layers.push_back((*it)->getData());

    return ret;
}

const NodeGpuData DataManager::
getData(const NodeGpuBuffer& gpuBuf) const
{
    NodeGpuData ret;
    ret.geometry = &gpuBuf.geometry->getData();
    ret.height   = &gpuBuf.height->getData();

    typedef NodeGpuBuffer::SubRegionBufferPtrs::const_iterator Iterator;
    for (Iterator it=gpuBuf.colors.begin(); it!=gpuBuf.colors.end(); ++it)
        ret.colors.push_back(&(*it)->getData());

    for (Iterator it=gpuBuf.layers.begin(); it!=gpuBuf.layers.end(); ++it)
        ret.layers.push_back(&(*it)->getData());

    ret.coverage = &gpuBuf.coverage->getData();
    ret.lineData = &gpuBuf.lineData->getData();

    return ret;
}

bool DataManager::
existsChildData(const NodeMainData& parent)
{
    const NodeData& node = *parent.node;

    for (int i=0; i<4; ++i)
    {
        if (node.demTile.children[i] != INVALID_TILEINDEX)
            return true;
    }

    typedef NodeData::Tiles::const_iterator Iterator;
    for (Iterator it=node.colorTiles.begin(); it!=node.colorTiles.end();
         ++it)
    {
        for (int i=0; i<4; ++i)
        {
            if (it->children[i] != INVALID_TILEINDEX)
                return true;
        }
    }

    for (Iterator it=node.layerTiles.begin(); it!=node.layerTiles.end();
         ++it)
    {
        for (int i=0; i<4; ++i)
        {
            if (it->children[i] != INVALID_TILEINDEX)
                return true;
        }
    }

    return false;
}

#define FIND_BUFFER(buf, cache, index)\
buf = cache.find(index);\
if (buf==NULL || !cache.isValid(buf))\
    return false;

bool DataManager::
find(const TreeIndex& index, NodeMainBuffer& mainBuf) const
{
    MainCache& mc = CACHE->getMainCache();

    FIND_BUFFER(    mainBuf.node,     mc.node, DataIndex(0, index))
    FIND_BUFFER(mainBuf.geometry, mc.geometry, DataIndex(0, index))
    FIND_BUFFER(  mainBuf.height,   mc.layerf, DataIndex(0, index))

    const int numColorLayers = static_cast<int>(colorFiles.size());
    mainBuf.colors.resize(numColorLayers, NULL);
    for (int l=0; l<numColorLayers; ++l)
    {
        FIND_BUFFER(mainBuf.colors[l], mc.color, DataIndex(l, index))
    }

    const int numFloatLayers = static_cast<int>(layerfFiles.size());
    mainBuf.layers.resize(numFloatLayers, NULL);
    for (int l=0; l<numFloatLayers; ++l)
    {
        FIND_BUFFER(mainBuf.layers[l], mc.layerf, DataIndex(l+1, index))
    }

    return true;
}


void DataManager::
startGpuBatch(GLContextData& contextData, const SurfaceApproximation& surface,
              Batch& batch)
{
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

    size_t numLayerBufs = numNodes * 3*colorFiles.size() + layerfFiles.size();
    gpuCache.layerf.reset(numLayerBufs);

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
isCurrent(const NodeMainBuffer& mainBuf) const
{
    return mainBuf.node->getFrameStamp() == CURRENT_FRAME;
}

bool DataManager::
isComplete(const NodeMainBuffer& mainBuf) const
{
    if (mainBuf.node==NULL || mainBuf.geometry==NULL || mainBuf.height==NULL)
        return false;

    typedef NodeMainBuffer::ColorBufferPtrs::const_iterator ColorIte;
    for (ColorIte it=mainBuf.colors.begin(); it!=mainBuf.colors.end(); ++it)
    {
        if (*it == NULL)
            return false;
    }

    typedef NodeMainBuffer::LayerBufferPtrs::const_iterator LayerIte;
    for (LayerIte it=mainBuf.layers.begin(); it!=mainBuf.layers.end(); ++it)
    {
        if (*it == NULL)
            return false;
    }

    return true;
}

void DataManager::
touch(NodeMainBuffer& mainBuf) const
{
    MainCache& mc = CACHE->getMainCache();
    mc.node.touch(mainBuf.node);
    mc.geometry.touch(mainBuf.geometry);
    mc.layerf.touch(mainBuf.height);

    typedef NodeMainBuffer::ColorBufferPtrs::iterator ColorIte;
    for (ColorIte it=mainBuf.colors.begin(); it!=mainBuf.colors.end(); ++it)
        mc.color.touch(*it);

    typedef NodeMainBuffer::LayerBufferPtrs::iterator LayerIte;
    for (LayerIte it=mainBuf.layers.begin(); it!=mainBuf.layers.end(); ++it)
        mc.layerf.touch(*it);
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


DataManager::GlItem::
GlItem() :
    sourceShaders(NULL), resetSourceShadersStamp(0)
{
}


#define GET_BUFFER(ret, cache, index, check)\
ret = cache.find(index);\
if (ret == NULL)\
    ret = cache.grabBuffer(check);

const NodeMainBuffer DataManager::
grabMainBuffer(const TreeIndex& index, bool current) const
{
    MainCache& mc = CACHE->getMainCache();

    NodeMainBuffer ret;
    GET_BUFFER(    ret.node,     mc.node, DataIndex(0,index), current);
    GET_BUFFER(ret.geometry, mc.geometry, DataIndex(0,index), current);
    GET_BUFFER(  ret.height,   mc.layerf, DataIndex(0,index), current);

    const int numColorLayers = static_cast<int>(colorFiles.size());
    ret.colors.resize(numColorLayers, NULL);
    for (int l=0; l<numColorLayers; ++l)
    {
        GET_BUFFER(ret.colors[l], mc.color, DataIndex(l, index), current)
    }

    const int numFloatLayers = static_cast<int>(layerfFiles.size());
    ret.layers.resize(numFloatLayers, NULL);
    for (int l=0; l<numFloatLayers; ++l)
    {
        GET_BUFFER(ret.layers[l], mc.layerf, DataIndex(l+1, index), current)
    }

    return ret;
}

void DataManager::
releaseMainBuffer(const TreeIndex& index, const NodeMainBuffer& buffer) const
{
    MainCache& mc = CACHE->getMainCache();

    if (buffer.node != NULL)
        mc.node.releaseBuffer(DataIndex(0,index), buffer.node);
    if (buffer.geometry != NULL)
        mc.geometry.releaseBuffer(DataIndex(0,index), buffer.geometry);
    if (buffer.height != NULL)
        mc.layerf.releaseBuffer(DataIndex(0,index), buffer.height);

    const int numColorLayers = static_cast<int>(buffer.colors.size());
    for (int l=0; l<numColorLayers; ++l)
    {
        if (buffer.colors[l] != NULL)
            mc.color.releaseBuffer(DataIndex(l,index), buffer.colors[l]);
    }

    const int numFloatLayers = static_cast<int>(buffer.layers.size());
    for (int l=0; l<numFloatLayers; ++l)
    {
        if (buffer.layers[l] != NULL)
            mc.layerf.releaseBuffer(DataIndex(l+1,index), buffer.layers[l]);
    }
}


#define STREAM(cacheType, cache, index, mainData, gpuData, format, type)\
{\
cacheType::BufferType* buf = cache.find(index);\
if (buf == NULL)\
{\
    buf = cache.grabBuffer(false);\
    if (buf == NULL)\
        return false;\
}\
gpuData = &buf->getData();\
if (cache.isGrabbed(buf) || !cache.isValid(buf))\
{\
    cache.stream(*gpuData, format, type, mainData);\
    CHECK_GLA;\
}\
cache.releaseBuffer(index, buf);\
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
    STREAM(GpuGeometryCache, cache.geometry, DataIndex(0,index), main.geometry,
           gpu.geometry, GL_RGB, GL_FLOAT)

//- handle the height data
    STREAM(GpuLayerfCache, cache.layerf, DataIndex(0,index), main.height,
           gpu.height, GL_RED, GL_FLOAT)

//- handle the color data
    int numColorLayers = static_cast<int>(main.colors.size());
    gpu.colors.resize(numColorLayers, NULL);
    for (int l=0; l<numColorLayers; ++l)
    {
        STREAM(GpuColorCache, cache.color, DataIndex(l,index),
               main.colors[l], gpu.colors[l], GL_RGB, GL_UNSIGNED_BYTE)
    }

//- handle the layer data
    int numFloatLayers = static_cast<int>(main.layers.size());
    gpu.layers.resize(numFloatLayers, NULL);
    for (int l=0; l<numFloatLayers; ++l)
    {
        STREAM(GpuLayerfCache, cache.layerf, DataIndex(l+1,index),
               main.layers[l], gpu.layers[l], GL_RED, GL_FLOAT)
    }

    //decorated vector art requires up-to-date line data and coverage textures
    if (SETTINGS->decorateVectorArt && main.node->lineNumSegments!=0)
    {
    //- handle the line data
        //acquire buffer
        GpuLineDataCache::BufferType* lineData =
            cache.lineData.find(DataIndex(0,index));
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
            cache.lineData.subStream(
                (SubRegion)(*gpu.lineData), 0, main.node->lineData.size(),
                GL_RGBA, GL_FLOAT, &main.node->lineData.front());
            CHECK_GLA;
            //stamp the age of the new data
            gpu.lineData->age = CURRENT_FRAME;
            updatedLine = true;
        }
        //validate buffer
        cache.lineData.releaseBuffer(DataIndex(0,index), lineData);

    //- handle the coverage data
        //acquire buffer
        GpuCoverageCache::BufferType* coverage =
            cache.coverage.find(DataIndex(0,index));
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
        cache.coverage.releaseBuffer(DataIndex(0,index), coverage);
    }

    return true;
}

void DataManager::
loadChild(Crusta* crusta, NodeMainData& parent,
          uint8 which,    NodeMainData& child)
{
    NodeData& parentNode = *parent.node;
    NodeData& childNode  = *child.node;

    //compute the child scopes
    Scope childScopes[4];
    parentNode.scope.split(childScopes);

    int numColorLayers = static_cast<int>(colorFiles.size());
    int numFloatLayers = static_cast<int>(layerfFiles.size());

//- Node data
    //clear the data layers
    childNode.colorTiles.resize(numColorLayers, NodeData::Tile());
    child.colors.resize(numColorLayers, NULL);
    childNode.layerTiles.resize(numFloatLayers, NodeData::Tile());
    child.layers.resize(numFloatLayers, NULL);

    //clear the old line data
    childNode.lineCoverage.clear();
    childNode.lineNumSegments = 0;
    childNode.lineData.clear();

    //initialize
    childNode.index     = parentNode.index.down(which);
    childNode.scope     = childScopes[which];

//- Geometry data
    generateGeometry(crusta, &childNode, child.geometry);

//- Topography data
    //topography reserves the first data id of the layerf cache
    childNode.demTile.dataId = 0;
    childNode.demTile.node   = parentNode.demTile.children[which];
    for (int i=0; i<4; ++i)
        childNode.demTile.children[i] = INVALID_TILEINDEX;

    sourceDem(&parentNode, parent.height, &childNode, child.height);

//- Texture color layer data
    for (int l=0; l<numColorLayers; ++l)
    {
        //generate and save the tile indices for this data
        NodeData::Tile& tile = childNode.colorTiles[l];
        tile.dataId = l;
        tile.node   = parentNode.colorTiles[l].children[which];
        for (int c=0; c<4; ++c)
            tile.children[c] = INVALID_TILEINDEX;

        //read in the color data
        sourceColor(&parentNode,   parent.colors[l],
                     &childNode, l, child.colors[l]);
    }

//- Layerf layer data
    for (int l=0; l<numFloatLayers; ++l)
    {
        //generate and save the tile indices for this data
        NodeData::Tile& tile = childNode.layerTiles[l];
        tile.dataId = l+1;
        tile.node   = parentNode.layerTiles[l].children[which];
        for (int c=0; c<4; ++c)
            tile.children[c] = INVALID_TILEINDEX;

        //read in the layer
        sourceLayerf(&parentNode,    parent.layers[l],
                      &childNode, l,  child.layers[l]);
    }

//- Finalize the node
    childNode.init(SETTINGS->globeRadius, crusta->getVerticalScale());

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
        v->position[0] = DemHeight::Type(g[0] - child->centroid[0]);
        v->position[1] = DemHeight::Type(g[1] - child->centroid[1]);
        v->position[2] = DemHeight::Type(g[2] - child->centroid[2]);
    }
}

template <typename PixelType>
inline void
sampleParentBase(int child, PixelType range[2], PixelType* dst,
                 const PixelType* const src, const PixelType& nodata)
{
    typedef PixelOps<PixelType> po;

    static const int offsets[4] = {
        0, (TILE_RESOLUTION-1)>>1, ((TILE_RESOLUTION-1)>>1)*TILE_RESOLUTION,
        ((TILE_RESOLUTION-1)>>1)*TILE_RESOLUTION + ((TILE_RESOLUTION-1)>>1) };
    int halfSize[2] = { (TILE_RESOLUTION+1)>>1, (TILE_RESOLUTION+1)>>1 };

    for (int y=0; y<halfSize[1]; ++y)
    {
        PixelType*       wbase = dst + y*2*TILE_RESOLUTION;
        const PixelType* rbase = src + y*TILE_RESOLUTION + offsets[child];

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
sampleParent(int child, DemHeight::Type range[2], DemHeight::Type* dst,
             const DemHeight::Type* const src, const DemHeight::Type& nodata)
{
    range[0] = Math::Constants<DemHeight::Type>::max;
    range[1] = Math::Constants<DemHeight::Type>::min;

    sampleParentBase(child, range, dst, src, nodata);
}

inline void
sampleParent(int child, TextureColor::Type* dst,
             const TextureColor::Type* const src,
             const TextureColor::Type& nodata)
{
    TextureColor::Type range[2] = {TextureColor::Type(0),TextureColor::Type(0)};
    sampleParentBase(child, range, dst, src, nodata);
}

inline void
sampleParent(int child, LayerDataf::Type* dst,
             const LayerDataf::Type* const src, const LayerDataf::Type& nodata)
{
    LayerDataf::Type range[2] = {0.0f, 0.0f};
    sampleParentBase(child, range, dst, src, nodata);
}

void DataManager::
sourceDem(const NodeData* const parent,
          const DemHeight::Type* const parentHeight,
          NodeData* child, DemHeight::Type* childHeight)
{
    typedef DemFile::File    File;
    typedef File::TileHeader TileHeader;

    DemHeight::Type* range = &child->elevationRange[0];

    if (child->demTile.node != INVALID_TILEINDEX)
    {
        TileHeader header;
        File* file = demFile->getPatch(child->index.patch);
        if (!file->readTile(child->demTile.node, child->demTile.children,
                            header, childHeight))
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
        {
            sampleParent(child->index.child, range, childHeight, parentHeight,
                         demNodata);
        }
        else
        {
            range[0] =  Math::Constants<DemHeight::Type>::max;
            range[1] = -Math::Constants<DemHeight::Type>::max;
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                childHeight[i] = demNodata;
        }
    }
}

void DataManager::
sourceColor(const NodeData* const parent,
            const TextureColor::Type* const parentColor,
            NodeData* child, uint8 layer,
            TextureColor::Type* childColor)
{
    typedef ColorFile::File File;

    if (child->colorTiles[layer].node != INVALID_TILEINDEX)
    {
        assert(layer < colorFiles.size());
        //get the color data into a temporary storage
        File* file = colorFiles[layer]->getPatch(child->index.patch);
        if (!file->readTile(child->colorTiles[layer].node,
                            child->colorTiles[layer].children, childColor))
        {
            Misc::throwStdErr("DataManager::sourceColor: Invalid Color "
                              "file: could not read node %s's data",
                              child->index.med_str().c_str());
        }
    }
    else
    {
        if (parent != NULL)
        {
            sampleParent(child->index.child, childColor, parentColor,
                         colorNodata);
        }
        else
        {
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                childColor[i] = colorNodata;
        }
    }
}

void DataManager::
sourceLayerf(const NodeData* const parent,
                  const LayerDataf::Type* const parentLayerf,
                  NodeData* child, uint8 layer, LayerDataf::Type* childLayerf)
{
    typedef LayerfFile::File File;

    if (child->layerTiles[layer].node != INVALID_TILEINDEX)
    {
        assert(layer<layerfFiles.size());
        File* file = layerfFiles[layer]->getPatch(child->index.patch);
        if (!file->readTile(child->layerTiles[layer].node,
                            child->layerTiles[layer].children,
                            childLayerf))
        {
            Misc::throwStdErr("DataManager::sourceLayerf: Invalid Layerf "
                              "file: could not read node %s's data",
                              child->index.med_str().c_str());
        }
    }
    else
    {
        if (parent != NULL)
        {
            sampleParent(child->index.child, childLayerf, parentLayerf,
                         layerfNodata);
        }
        else
        {
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                childLayerf[i] = layerfNodata;
        }
    }
}


void DataManager::
startFetchThread()
{
    terminateFetch = false;
    fetchThread.start(this, &DataManager::fetchThreadFunc);
}

void DataManager::
terminateFetchThread()
{
    if (!fetchThread.isJoined())
    {
        //let the fetch thread know that it should terminate
        terminateFetch = true;
        //make sure the thread is not stuck waiting for requests
        fetchCond.signal();
        //wait for the termination
        fetchThread.join();
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
            //terminate before trying to fetch more?
            if (terminateFetch)
                return NULL;

            //wait for new requests if there are none pending
            if (fetchRequests.empty())
                fetchCond.wait(requestMutex);

            //was the signal actually to terminate? If not grab the request
            if (terminateFetch)
                return NULL;
            else
                fetch = &fetchRequests.front();
        }

        //fetch it
        NodeMainData parentData = getData(fetch->parent);
        NodeMainData childData  = getData(fetch->child);
        loadChild(fetch->crusta, parentData, fetch->which, childData);
    }

    return NULL;
}


void DataManager::
initContext(GLContextData& contextData) const
{
    GlItem* glItem = new GlItem;
    contextData.addDataItem(this, glItem);
}


DataManager* DATAMANAGER = NULL;


END_CRUSTA
