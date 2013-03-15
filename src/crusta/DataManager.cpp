#include <crusta/DataManager.h>

#include <sstream>

#include <Math/Constants.h>

#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>
#include <crustacore/PixelOps.h>
#include <crustacore/PolyhedronLoader.h>
#include <crusta/QuadCache.h>
#include <crusta/QuadTerrain.h>
#include <crustacore/Triacontahedron.h>

#include <Vrui/Vrui.h>


namespace crusta {


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
        uint8_t iChild) :
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

    delete[] tempGeometryBuf;
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
                demFile = new DemFile(false);
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
            ColorFile* file = new ColorFile(false);
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
            LayerfFile* file = new LayerfFile(false);
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
    {
        Threads::Mutex::Lock lock(requestMutex);
        childRequests.clear();
    }

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
sampleLayerf(int layerIndex, const SurfacePoint& point)
{
    layerIndex -= getNumColorLayers();
    DataIndex index(layerIndex, point.nodeIndex);

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
    name##Buf = cache.grabBuffer(CURRENT_FRAME);\
if (name##Buf == NULL)\
   {\
       Misc::throwStdErr("DataManager::loadRoot: failed to acquire %s cache "\
                         "buffer for root node of patch %d", #name,\
                         rootIndex.patch());\
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
    {
        //make sure merging of the requests is done one at a time
        Threads::Mutex::Lock lock(requestMutex);
        addRequest(req);
    }
    fetchCond.signal();
}

void DataManager::
request(const Requests& reqs)
{
    {
        //make sure merging of the requests is done one at a time
        Threads::Mutex::Lock lock(requestMutex);
        for (Requests::const_iterator it=reqs.begin(); it!=reqs.end(); ++it)
            addRequest(*it);
        if (!childRequests.empty())
            fetchCond.signal();
    }
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
    assert(remainingBatchIndices.empty());

    //stream nodes cached nodes and reserved the indices of the missing
    size_t numNodes = surface.visibles.size();
    for (size_t i=0; i<numNodes; ++i)
    {
        BatchElement batchel;
        batchel.main = surface.visible(i);
        NodeGpuBuffer gpuBuf;
        if (findGpuBuffer(contextData, batchel.main, gpuBuf))
        {
            //make sure the data is up to date
            streamGpuData(contextData, batchel, gpuBuf);
            CHECK_GLA;
            //add to the batch
            batch.push_back(batchel);
        }
        else
            remainingBatchIndices.push_back(i);
    }

    //add missing nodes as much as the cache will allow
    while (!remainingBatchIndices.empty())
    {
        BatchElement batchel;
        batchel.main = surface.visible(remainingBatchIndices.back());
        NodeGpuBuffer gpuBuf;
        if (grabGpuBuffer(contextData, batchel.main, gpuBuf))
        {
            //make sure the data is up to date
            streamGpuData(contextData, batchel, gpuBuf);
            CHECK_GLA;
            //add to the batch
            batch.push_back(batchel);
            //remove the index from the remaining set
            remainingBatchIndices.pop_back();
        }
        else
            break;
    }
}

void DataManager::
nextGpuBatch(GLContextData& contextData, const SurfaceApproximation& surface,
             Batch& batch)
{
    batch.clear();

    //are any more batches even possible
    size_t numNodes = remainingBatchIndices.size();
    if (numNodes == 0)
        return;

    //try to reset enough cache entries for the remaining nodes
    GpuCache& gpuCache = CACHE->getGpuCache(contextData);
    gpuCache.geometry.ageMRU(numNodes, LAST_FRAME);
    gpuCache.color.ageMRU(numNodes * colorFiles.size(), LAST_FRAME);

    size_t numLayerBufs = numNodes * 3*colorFiles.size() + layerfFiles.size();
    gpuCache.layerf.ageMRU(numLayerBufs, LAST_FRAME);

    gpuCache.coverage.ageMRU(numNodes, LAST_FRAME);
    gpuCache.lineData.ageMRU(numNodes, LAST_FRAME);

    //add missing nodes as much as the cache will allow
    while (!remainingBatchIndices.empty())
    {
        BatchElement batchel;
        batchel.main = surface.visible(remainingBatchIndices.back());
        NodeGpuBuffer gpuBuf;
        if (grabGpuBuffer(contextData, batchel.main, gpuBuf))
        {
            //make sure the data is up to date
            streamGpuData(contextData, batchel, gpuBuf);
            CHECK_GLA;
            //add to the batch
            batch.push_back(batchel);
            //remove the index from the remaining set
            remainingBatchIndices.pop_back();
        }
        else
            break;
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


DataManager::GlItem::
GlItem() :
    sourceShaders(NULL), resetSourceShadersStamp(0)
{
}


#define GRABMAINBUFFER(ret, cache, index, older)\
ret = cache.find(index);\
if (ret == NULL)\
{\
    ret = cache.grabBuffer(older);\
    if (ret == NULL)\
        return false;\
}

bool DataManager::
grabMainBuffer(const TreeIndex& index, const FrameStamp older,
               NodeMainBuffer& mainBuf) const
{
    MainCache& mc = CACHE->getMainCache();

    GRABMAINBUFFER(    mainBuf.node,     mc.node, DataIndex(0,index), older);
    GRABMAINBUFFER(mainBuf.geometry, mc.geometry, DataIndex(0,index), older);
    GRABMAINBUFFER(  mainBuf.height,   mc.layerf, DataIndex(0,index), older);

    size_t numColorLayers = colorFiles.size();
    mainBuf.colors.resize(numColorLayers, NULL);
    for (size_t i=0; i<numColorLayers; ++i)
    {
        GRABMAINBUFFER(mainBuf.colors[i],mc.color,DataIndex(i,index),older);
    }

    size_t numFloatLayers = layerfFiles.size();
    mainBuf.layers.resize(numFloatLayers, NULL);
    for (size_t i=0; i<numFloatLayers; ++i)
    {
        GRABMAINBUFFER(mainBuf.layers[i],mc.layerf,DataIndex(i+1,index),older);
    }

    return true;
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


#define FINDGPUBUFFER(buf, cache, index)\
{\
    buf = cache.find(index);\
    if (buf == NULL)\
        return false;\
}

bool DataManager::
findGpuBuffer(GLContextData& contextData, const NodeMainData& main,
              NodeGpuBuffer& gpuBuf) const
{
    const TreeIndex& index = main.node->index;
    GpuCache& cache = CACHE->getGpuCache(contextData);

    //geometry
    FINDGPUBUFFER(gpuBuf.geometry, cache.geometry, DataIndex(0,index));
    //height
    FINDGPUBUFFER(gpuBuf.height, cache.layerf, DataIndex(0,index));
    //colors
    size_t numColorLayers = main.colors.size();
    gpuBuf.colors.resize(numColorLayers, NULL);
    for (size_t i=0; i<numColorLayers; ++i)
        FINDGPUBUFFER(gpuBuf.colors[i], cache.color, DataIndex(i,index));
    //layerfs
    size_t numFloatLayers = main.layers.size();
    gpuBuf.layers.resize(numFloatLayers, NULL);
    for (size_t i=0; i<numFloatLayers; ++i)
        FINDGPUBUFFER(gpuBuf.layers[i], cache.layerf, DataIndex(i+1,index));
    //decorated vector art data
    if (SETTINGS->lineDecorated && main.node->lineNumSegments!=0)
    {
        //line data
        FINDGPUBUFFER(gpuBuf.lineData, cache.lineData, DataIndex(0,index));
        //coverage
        FINDGPUBUFFER(gpuBuf.coverage, cache.coverage, DataIndex(0,index));
    }

    //successfully found all the components of the buffer
    return true;
}

#define GRABGPUBUFFER(buf, cache, index)\
{\
    buf = cache.find(index);\
    if (buf == NULL)\
    {\
        buf = cache.grabBuffer(CURRENT_FRAME);\
        if (buf == NULL)\
            return false;\
    }\
}

bool DataManager::
grabGpuBuffer(GLContextData &contextData, const NodeMainData &main,
              NodeGpuBuffer &gpuBuf) const
{
    const TreeIndex& index = main.node->index;
    GpuCache& cache = CACHE->getGpuCache(contextData);

    //geometry
    GRABGPUBUFFER(gpuBuf.geometry, cache.geometry, DataIndex(0,index));
    //height
    GRABGPUBUFFER(gpuBuf.height, cache.layerf, DataIndex(0,index));
    //colors
    size_t numColorLayers = main.colors.size();
    gpuBuf.colors.resize(numColorLayers, NULL);
    for (size_t i=0; i<numColorLayers; ++i)
        GRABGPUBUFFER(gpuBuf.colors[i], cache.color, DataIndex(i,index));
    //layerfs
    size_t numFloatLayers = main.layers.size();
    gpuBuf.layers.resize(numFloatLayers, NULL);
    for (size_t i=0; i<numFloatLayers; ++i)
        GRABGPUBUFFER(gpuBuf.layers[i], cache.layerf, DataIndex(i+1,index));
    //decorated vector art data
    if (SETTINGS->lineDecorated && main.node->lineNumSegments!=0)
    {
        //line data
        GRABGPUBUFFER(gpuBuf.lineData, cache.lineData, DataIndex(0,index));
        //coverage
        GRABGPUBUFFER(gpuBuf.coverage, cache.coverage, DataIndex(0,index));
    }

    //successfully found all the components of the buffer
    return true;
}

#define STREAM(buf, cache, index, mainData, gpuData, format, type)\
{\
gpuData = &buf->getData();\
if (!cache.isValid(buf))\
{\
    cache.stream(*gpuData, format, type, mainData);\
    CHECK_GLA;\
}\
cache.releaseBuffer(index, buf);\
}

void DataManager::
streamGpuData(GLContextData& contextData, BatchElement& batchel,
              NodeGpuBuffer& gpuBuf)
{
    GpuCache& cache        = CACHE->getGpuCache(contextData);
    NodeMainData& main     = batchel.main;
    NodeGpuData&  gpu      = batchel.gpu;
    const TreeIndex& index = main.node->index;

    CHECK_GLA;
//- handle the geometry data
    STREAM(gpuBuf.geometry, cache.geometry, DataIndex(0,index), main.geometry,
           gpu.geometry, GL_RGB, GL_FLOAT)

//- handle the height data
    STREAM(gpuBuf.height, cache.layerf, DataIndex(0,index), main.height,
           gpu.height, GL_RED, GL_FLOAT)

//- handle the color data
    size_t numColorLayers = main.colors.size();
    gpu.colors.resize(numColorLayers, NULL);
    for (size_t i=0; i<numColorLayers; ++i)
    {
        STREAM(gpuBuf.colors[i], cache.color, DataIndex(i,index),
               main.colors[i], gpu.colors[i], GL_RGB, GL_UNSIGNED_BYTE)
    }

//- handle the layer data
    size_t numFloatLayers = main.layers.size();
    gpu.layers.resize(numFloatLayers, NULL);
    for (size_t i=0; i<numFloatLayers; ++i)
    {
        STREAM(gpuBuf.layers[i], cache.layerf, DataIndex(i+1,index),
               main.layers[i], gpu.layers[i], GL_RED, GL_FLOAT)
    }

    //decorated vector art requires up-to-date line data and coverage textures
    if (SETTINGS->lineDecorated && main.node->lineNumSegments!=0)
    {
    //- handle the line data
        //update data
        gpu.lineData = &gpuBuf.lineData->getData();
        bool updatedLine = false;
        if (!cache.lineData.isValid(gpuBuf.lineData)  ||
            gpu.lineData->age < main.node->lineDataStamp)
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
        cache.lineData.releaseBuffer(DataIndex(0,index), gpuBuf.lineData);

    //- handle the coverage data
        //update the data
        gpu.coverage = &gpuBuf.coverage->getData();
        if (updatedLine || !cache.coverage.isValid(gpuBuf.coverage))
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
        cache.coverage.releaseBuffer(DataIndex(0,index), gpuBuf.coverage);
    }
}

void DataManager::
loadChild(Crusta* crusta, NodeMainData& parent,
          uint8_t which,    NodeMainData& child)
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
        File* file = demFile->getPatch(child->index.patch());
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
            sampleParent(child->index.child(), range,
                         childHeight, parentHeight, demNodata);
        }
        else
        {
            range[0] =  Math::Constants<DemHeight::Type>::max;
            range[1] = -Math::Constants<DemHeight::Type>::max;
            for (size_t i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                childHeight[i] = demNodata;
        }
    }
}

void DataManager::
sourceColor(const NodeData* const parent,
            const TextureColor::Type* const parentColor,
            NodeData* child, uint8_t layer,
            TextureColor::Type* childColor)
{
    typedef ColorFile::File File;

    if (child->colorTiles[layer].node != INVALID_TILEINDEX)
    {
        assert(layer < colorFiles.size());
        //get the color data into a temporary storage
        File* file = colorFiles[layer]->getPatch(child->index.patch());
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
            sampleParent(child->index.child(), childColor, parentColor,
                         colorNodata);
        }
        else
        {
            for (size_t i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                childColor[i] = colorNodata;
        }
    }
}

void DataManager::
sourceLayerf(const NodeData* const parent,
                  const LayerDataf::Type* const parentLayerf,
                  NodeData* child, uint8_t layer, LayerDataf::Type* childLayerf)
{
    typedef LayerfFile::File File;

    if (child->layerTiles[layer].node != INVALID_TILEINDEX)
    {
        assert(layer<layerfFiles.size());
        File* file = layerfFiles[layer]->getPatch(child->index.patch());
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
            sampleParent(child->index.child(), childLayerf, parentLayerf,
                         layerfNodata);
        }
        else
        {
            for (size_t i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                childLayerf[i] = layerfNodata;
        }
    }
}


void DataManager::
addRequest(Request req)
{
    typedef std::list<Request> ReqList;

    //find where to insert the request and coalesce duplicates
    ReqList::iterator insertIte;
    for (insertIte=childRequests.begin(); insertIte!=childRequests.end();)
    {
        //remove existing entry, but update the LOD as necessary
        if (req == *insertIte)
        {
            req.lod = std::min(req.lod, insertIte->lod);
            childRequests.erase(insertIte++);
            continue;
        }
        //stop at the right LOD insertion point
        if (req > *insertIte)
            break;
        else
            ++insertIte;
    }

    //insert the request
    childRequests.insert(insertIte, req);
    //keep the request list manageable
    while (static_cast<int>(childRequests.size()) >
           SETTINGS->dataManMaxFetchRequests)
    {
        childRequests.pop_front();
    }
    assert(!childRequests.empty());
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
    Request req;
    while (true)
    {
    //-- grab a request from the pending list
        {
            //make sure there are requests available
            Threads::Mutex::Lock lock(requestMutex);
            if (childRequests.empty())
                fetchCond.wait(requestMutex);
            //terminate before trying to fetch more?
            if (terminateFetch)
                return NULL;
            //grab the request
            assert(!childRequests.empty());
            req = childRequests.back();
            childRequests.pop_back();
        }

    //-- try to grab a cache entry to satisfy the request
        TreeIndex childIndex=req.parent.node->getData().index.down(req.child);
        /* Because the frame swaps are no synchronized with this thread, the
           grab could occur right after the swap (i.e., CURRENT_FRAME set to
           the new timestamp), then all the cache entries would be valid
           candidates for reuse. To prevent tossing out data that the current
           evaluation of the surface approximation would retain from the
           previous frame, we restrict candidates to ones that have been
           neglected for at least two frames already */
        NodeMainBuffer mainBuf;
        if (!grabMainBuffer(childIndex, LAST_FRAME, mainBuf))
        {
            //we couldn't secure a buffer from the cache bail on this request
CRUSTA_DEBUG(11, CRUSTA_DEBUG_OUT <<
"FetchThread: no more room in the cache for new request\n";)
            continue;
        }

    //-- fetch it
        NodeMainData parentData = getData(req.parent);
        NodeMainData childData  = getData(mainBuf);
        loadChild(req.crusta, parentData, req.child, childData);

    //-- make it available
        releaseMainBuffer(childIndex, mainBuf);
CRUSTA_DEBUG(14, CRUSTA_DEBUG_OUT <<
"FetchThread: request for Index " << childIndex.med_str() << ":" <<
req.child << " processed\n";)
        Vrui::requestUpdate();
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


} //namespace crusta
