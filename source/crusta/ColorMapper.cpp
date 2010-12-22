#include <crusta/ColorMapper.h>


#include <crusta/DataManager.h>
#include <crusta/CrustaSettings.h>


BEGIN_CRUSTA


ColorMapper::MainLayer::
MainLayer(FrameStamp initialStamp) :
    mapColorStamp(initialStamp), mapRangeStamp(initialStamp)
{
}


ColorMapper::
ColorMapper() :
    mapperConfigurationStamp(0), gpuLayersStamp(0), clampMap(false),
    activeMapIndex(-1)
{
}


void ColorMapper::
load()
{
    unload();

    configureMainLayers();
    //flag the gpu representation for update
    gpuLayersStamp = CURRENT_FRAME;
    //flag the update of the configuration to external modules
    mapperConfigurationStamp = CURRENT_FRAME;
}

void ColorMapper::
unload()
{
    //get rid of the old representation
    mainLayers.clear();

    //flag the gpu representation for update
    gpuLayersStamp = CURRENT_FRAME;
    //flag the update of the configuration to external modules
    mapperConfigurationStamp = CURRENT_FRAME;
}

void ColorMapper::
touchColor(int mapIndex)
{
    assert(mapIndex>=0 && mapIndex<static_cast<int>(mainLayers.size()));
    mainLayers[mapIndex].mapColorStamp = CURRENT_FRAME;
}

void ColorMapper::
touchRange(int mapIndex)
{
    assert(mapIndex>=0 && mapIndex<static_cast<int>(mainLayers.size()));
    mainLayers[mapIndex].mapRangeStamp = CURRENT_FRAME;
}


FrameStamp ColorMapper::
getMapperConfigurationStamp() const
{
    return mapperConfigurationStamp;
}

int ColorMapper::
getNumColorMaps() const
{
    return static_cast<int>(mainLayers.size());
}

int ColorMapper::
getHeightColorMapIndex() const
{
    if (DATAMANAGER->hasDem())
        return 0;
    else
        return -1;
}

Misc::ColorMap& ColorMapper::
getColorMap(int mapIndex)
{
    assert(mapIndex>=0 && mapIndex<static_cast<int>(mainLayers.size()));
    return mainLayers[mapIndex].mapColor;
}

const Misc::ColorMap& ColorMapper::
getColorMap(int mapIndex) const
{
    assert(mapIndex>=0 && mapIndex<static_cast<int>(mainLayers.size()));
    return mainLayers[mapIndex].mapColor;
}

int ColorMapper::
getActiveMap() const
{
    return activeMapIndex;
}

void ColorMapper::
setActiveMap(int mapIndex)
{
    assert(mapIndex==-1 ||
           (mapIndex>=0 && mapIndex<static_cast<int>(mainLayers.size())));
    activeMapIndex = mapIndex;
}

void ColorMapper::
setClamping(bool clamp)
{
    clampMap = clamp;

    //flag the gpu representation for update
    gpuLayersStamp = CURRENT_FRAME;
    //flag the update of the configuration to external modules
    mapperConfigurationStamp = CURRENT_FRAME;
}


ShaderDataSource*  ColorMapper::
getColorSource(GLContextData& contextData)
{
    GlItem* glItem = contextData.retrieveDataItem<GlItem>(this);
    return &glItem->mixer;
}


#define GRAB_BUFFER(cacheType, name, cache, index)\
cacheType::BufferType* name##Buf = cache.find(index);\
if (name##Buf == NULL)\
    name##Buf = cache.grabBuffer(false);\
if (name##Buf == NULL)\
{\
    Misc::throwStdErr("ColorMapper::configureGpuLayers: failed to acquire %s "\
                      "color map buffer", #name);\
}\
cacheType::BufferType::DataType& name##Data = name##Buf->getData();

#define RELEASE_PIN_BUFFER(cache, index, buffer)\
cache.releaseBuffer(index, buffer);\
cache.pin(buffer);


void ColorMapper::
configureShaders(GLContextData& contextData)
{
    GlItem& gl = *contextData.retrieveDataItem<GlItem>(this);

    //check if reconfiguration is required
    if (gl.layersStamp >= gpuLayersStamp)
        return;

//- clear the old storage
    gl.mapCache.clear();
    gl.colors.clear();
    gl.layers.clear();

//- regenerate the layer regions and shaders
    int mapId = 0;
    //grab the data sources from the data manager
    DataManager::SourceShaders& sources =
        DATAMANAGER->getSourceShaders(contextData);
    //process the dem layer
    if (DATAMANAGER->hasDem())
    {
        DataIndex index(mapId, TreeIndex(0));
        GRAB_BUFFER(GpuColorMapCache, dem, gl.mapCache, index)
        gl.layers.push_back(GpuLayer(demData, &sources.height, clampMap));
        RELEASE_PIN_BUFFER(gl.mapCache, index, demBuf)
        ++mapId;
    }
    //process all the data layers
    typedef DataManager::Shader2dAtlasDataSources::iterator Iterator;
    for (Iterator it=sources.layers.begin(); it!=sources.layers.end();
         ++it, ++mapId)
    {
        DataIndex index(mapId, TreeIndex(0));
        GRAB_BUFFER(GpuColorMapCache, layerf, gl.mapCache, index)
        gl.layers.push_back(GpuLayer(layerfData, &(*it), clampMap));
        RELEASE_PIN_BUFFER(gl.mapCache, index, layerfBuf);
    }
    //process all the color layers
    for (Iterator it=sources.colors.begin(); it!=sources.colors.end(); ++it)
        gl.colors.push_back(ShaderColorReader(&(*it)));


#if 1
//- feed all the non-topography layerfs to the multiplier
    gl.multiplier.clear();

    int demOffset       = DATAMANAGER->hasDem() ? 1 : 0;
    int numLayerfLayers = static_cast<int>(gl.layers.size());
    for (int i=demOffset; i<numLayerfLayers; ++i)
        gl.multiplier.addSource(&gl.layers[i].mapShader);


//- reconnect the mixer
    gl.mixer.clear();
    for (ShaderColorReaders::iterator it=gl.colors.begin(); it!=gl.colors.end();
         ++it)
    {
        gl.mixer.addSource(&(*it));
    }

    if (DATAMANAGER->hasDem())
        gl.mixer.addSource(&gl.layers[0].mapShader);

    int numAuxiliaryLayerfs = numLayerfLayers - demOffset;
    if (numAuxiliaryLayerfs>0)
        gl.mixer.addSource(&gl.multiplier);
#else
//- reconnect the mixer
    gl.mixer.clear();
    for (ShaderColorReaders::iterator it=gl.colors.begin(); it!=gl.colors.end();
         ++it)
    {
        gl.mixer.addSource(&(*it));
    }
    for (GpuLayers::iterator it=gl.layers.begin(); it!=gl.layers.end(); ++it)
    {
        gl.mixer.addSource(&(it->mapShader));
    }
#endif

//- validate the current configuration
    gl.layersStamp = CURRENT_FRAME;

    CHECK_GLA
}

void ColorMapper::
updateShaders(GLContextData& contextData)
{
    GlItem& gl = *contextData.retrieveDataItem<GlItem>(this);

    int numLayers = static_cast<int>(mainLayers.size());
    for (int l=0; l<numLayers; ++l)
    {
        //get the main and gpu layer
        MainLayer& mainLayer = mainLayers[l];
        GpuLayer&  gpuLayer  = gl.layers[l];
        //stream the color data to the GPU if needed
        if (gpuLayer.mapColorStamp < mainLayer.mapColorStamp)
        {
            assert(mainLayer.mapColor.getPoints().size() > 1);
            Misc::ColorMap::DiscreteMap discrete(SETTINGS->colorMapTexSize);
            mainLayer.mapColor.discretize(discrete);
            gl.mapCache.stream(
                (SubRegion)gpuLayer.mapShader.getColorMapRegion(), GL_RGBA,
                GL_FLOAT, discrete.front().getRgba());
            CHECK_GLA
            gpuLayer.mapColorStamp = CURRENT_FRAME;
        }
        //update the range used in the shader if needed
        if (gpuLayer.mapRangeStamp < mainLayer.mapRangeStamp)
        {
            Misc::ColorMap::ValueRange vr = mainLayer.mapColor.getValueRange();
            gpuLayer.mapShader.setScalarRange(vr.min, vr.max);
            CHECK_GLA
            gpuLayer.mapRangeStamp = CURRENT_FRAME;
        }
    }
    CHECK_GLA
}
void ColorMapper::
bindColorMaps(GLContextData& contextData)
{
    GlItem* gl = contextData.retrieveDataItem<GlItem>(this);
    gl->mapCache.bind();
}



ColorMapper::GpuLayer::
GpuLayer(const SubRegion& region, ShaderDataSource* source, bool clamp) :
    mapShader("colorMapTex", region, source, clamp),
    mapColorStamp(0), mapRangeStamp(0)
{
}

ColorMapper::GlItem::
GlItem() :
    layersStamp(0)
{
}

void ColorMapper::
configureMainLayers()
{
    //create the new colorMaps with the current stamp to trigger their upload
    int demOffset = DATAMANAGER->hasDem() ? 1 : 0;
    int numLayers = DATAMANAGER->getNumLayerfLayers();
    numLayers    += demOffset;
    mainLayers.resize(numLayers, MainLayer(CURRENT_FRAME));

    //initialize all additional layers as transparent
    for (int l=0; l<numLayers; ++l)
        mainLayers[l].mapColor = Misc::ColorMap::black;
}


void ColorMapper::
initContext(GLContextData& contextData) const
{
    GlItem* glItem = new GlItem;
    glItem->mapCache.init("GpuColorMapCache", SETTINGS->dataManMaxDataLayers,
                          SETTINGS->colorMapTexSize, GL_RGBA, GL_LINEAR);
    contextData.addDataItem(this, glItem);
}





ColorMapper* COLORMAPPER = NULL;


END_CRUSTA
