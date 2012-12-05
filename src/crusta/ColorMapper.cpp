#include <crusta/ColorMapper.h>


#include <crusta/DataManager.h>
#include <crusta/CrustaSettings.h>


BEGIN_CRUSTA


ColorMapper::MainLayer::
MainLayer() :
    isVisible(true)
{
}

ColorMapper::MainFloatLayer::
MainFloatLayer(FrameStamp initialStamp) :
    isClamped(false),
    mapColorStamp(initialStamp), mapRangeStamp(initialStamp)
{
}


ColorMapper::
ColorMapper() :
    mapperConfigurationStamp(0), gpuLayersStamp(0), activeLayerIndex(-1)
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
    mainColorLayers.clear();
    mainFloatLayers.clear();

    //flag the gpu representation for update
    gpuLayersStamp = CURRENT_FRAME;
    //flag the update of the configuration to external modules
    mapperConfigurationStamp = CURRENT_FRAME;
}

void ColorMapper::
touchColor(int layerIndex)
{
    assert(layerIndex>=static_cast<int>(mainColorLayers.size()) &&
           layerIndex<static_cast<int>(mainColorLayers.size() +
                                     mainFloatLayers.size()));
    layerIndex -= mainColorLayers.size();
    mainFloatLayers[layerIndex].mapColorStamp = CURRENT_FRAME;
}

void ColorMapper::
touchRange(int layerIndex)
{
    assert(layerIndex>=static_cast<int>(mainColorLayers.size()) &&
           layerIndex<static_cast<int>(mainColorLayers.size() +
                                     mainFloatLayers.size()));
    layerIndex -= mainColorLayers.size();
    mainFloatLayers[layerIndex].mapRangeStamp = CURRENT_FRAME;
}


void ColorMapper::
setVisible(int layerIndex, bool visible)
{
    int numLayers = static_cast<int>(mainColorLayers.size() +
                                     mainFloatLayers.size());
    if (layerIndex<0 || layerIndex>=numLayers)
        return;

    if (layerIndex < static_cast<int>(mainColorLayers.size()))
        mainColorLayers[layerIndex].isVisible = visible;
    else
        mainFloatLayers[layerIndex-mainColorLayers.size()].isVisible = visible;

    //flag the gpu representation for update
    gpuLayersStamp = CURRENT_FRAME;
    //flag the update of the configuration to external modules
    mapperConfigurationStamp = CURRENT_FRAME;
}

bool ColorMapper::
isVisible(int layerIndex) const
{
    int numLayers = static_cast<int>(mainColorLayers.size() +
                                     mainFloatLayers.size());
    if (layerIndex<0 || layerIndex>=numLayers)
        return false;

    if (layerIndex < static_cast<int>(mainColorLayers.size()))
        return mainColorLayers[layerIndex].isVisible;
    else
        return mainFloatLayers[layerIndex-mainColorLayers.size()].isVisible;
}

void ColorMapper::
setClamping(int layerIndex, bool clamp)
{
    if (layerIndex<static_cast<int>(mainColorLayers.size()) ||
        layerIndex>=static_cast<int>(mainColorLayers.size() +
                                     mainFloatLayers.size()))
    {
        return;
    }
    layerIndex -= mainColorLayers.size();

    mainFloatLayers[layerIndex].isClamped = clamp;

    //flag the gpu representation for update
    gpuLayersStamp = CURRENT_FRAME;
    //flag the update of the configuration to external modules
    mapperConfigurationStamp = CURRENT_FRAME;
}

bool ColorMapper::
isClamped(int layerIndex) const
{
    if (layerIndex<static_cast<int>(mainColorLayers.size()) ||
        layerIndex>=static_cast<int>(mainColorLayers.size() +
                                     mainFloatLayers.size()))
    {
        return false;
    }
    layerIndex -= mainColorLayers.size();

    return mainFloatLayers[layerIndex].isClamped;
}

FrameStamp ColorMapper::
getMapperConfigurationStamp() const
{
    return mapperConfigurationStamp;
}

static std::string
getFileName(const std::string& path)
{
    std::string name;
    //remove the parent path
    size_t delim = path.find_last_of("/\\");
    if (delim != std::string::npos)
        name = path.substr(delim+1);
    else
        name = path;

    return name;
}

ColorMapper::Strings ColorMapper::
getLayerNames() const
{
    Strings names;

    const Strings& colorNames = DATAMANAGER->getColorFilePaths();
    for (size_t i=0; i<colorNames.size(); ++i)
        names.push_back(getFileName(colorNames[i]));

    if (DATAMANAGER->hasDem())
        names.push_back(getFileName(DATAMANAGER->getDemFilePath()));

    const Strings& floatNames = DATAMANAGER->getLayerfFilePaths();
    for (size_t i=0; i<floatNames.size(); ++i)
        names.push_back(getFileName(floatNames[i]));

    return names;
}

int ColorMapper::
getNumColorMaps() const
{
    return static_cast<int>(mainFloatLayers.size());
}

int ColorMapper::
getHeightColorMapIndex() const
{
    if (DATAMANAGER->hasDem())
        return mainColorLayers.size();
    else
        return -1;
}

Misc::ColorMap& ColorMapper::
getColorMap(int layerIndex)
{
    assert(layerIndex>=static_cast<int>(mainColorLayers.size()) &&
           layerIndex<static_cast<int>(mainColorLayers.size() +
                                     mainFloatLayers.size()));
    layerIndex -= mainColorLayers.size();
    return mainFloatLayers[layerIndex].mapColor;
}

const Misc::ColorMap& ColorMapper::
getColorMap(int layerIndex) const
{
    assert(layerIndex>=static_cast<int>(mainColorLayers.size()) &&
           layerIndex<static_cast<int>(mainFloatLayers.size()));
    layerIndex -= mainColorLayers.size();
    return mainFloatLayers[layerIndex].mapColor;
}

bool ColorMapper::
isColorLayer(int layerIndex) const
{
    return layerIndex>=0 && layerIndex<static_cast<int>(mainColorLayers.size());
}

bool ColorMapper::
isFloatLayer(int layerIndex) const
{
    return layerIndex>=static_cast<int>(mainColorLayers.size()) &&
           layerIndex<static_cast<int>(mainColorLayers.size() +
                                       mainFloatLayers.size());
}


int ColorMapper::
getActiveLayer() const
{
    return activeLayerIndex;
}

void ColorMapper::
setActiveLayer(int layerIndex)
{
    activeLayerIndex = layerIndex;
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
    name##Buf = cache.grabBuffer(CURRENT_FRAME);\
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
    gl.floatLayers.clear();

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
        gl.floatLayers.push_back(GpuFloatLayer(
            demData, &sources.height, mainFloatLayers[mapId].isClamped));
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
        gl.floatLayers.push_back(GpuFloatLayer(
            layerfData, &(*it), mainFloatLayers[mapId].isClamped));
        RELEASE_PIN_BUFFER(gl.mapCache, index, layerfBuf);
    }
    //process all the color layers
    for (Iterator it=sources.colors.begin(); it!=sources.colors.end(); ++it)
        gl.colors.push_back(ShaderColorReader(&(*it)));


#if 0
//- feed all the non-topography layerfs to the multiplier
    gl.multiplier.clear();

    int demOffset       = DATAMANAGER->hasDem() ? 1 : 0;
    int numLayerfLayers = static_cast<int>(gl.floatLayers.size());
    for (int i=demOffset; i<numLayerfLayers; ++i)
    {
        if (mainFloatLayers[i].isVisible)
            gl.multiplier.addSource(&gl.floatLayers[i].mapShader);
    }


//- reconnect the mixer
    gl.mixer.clear();
    for (size_t i=0; i<gl.colors.size(); ++i)
    {
        if (mainColorLayers[i].isVisible)
            gl.mixer.addSource(&gl.colors[i]);
    }

    if (DATAMANAGER->hasDem())
    {
        if (mainFloatLayers[0].isVisible)
            gl.mixer.addSource(&gl.floatLayers[0].mapShader);
    }

    int numAuxiliaryLayerfs = numLayerfLayers - demOffset;
    if (numAuxiliaryLayerfs>0)
        gl.mixer.addSource(&gl.multiplier);
#else
//- reconnect the mixer
    gl.mixer.clear();

    int numColorLayers = static_cast<int>(mainColorLayers.size());
    for (int i=0; i<numColorLayers; ++i)
    {
        if (mainColorLayers[i].isVisible)
            gl.mixer.addSource(&gl.colors[i]);
    }

    int numFloatLayers = static_cast<int>(mainFloatLayers.size());
    for (int i=0; i<numFloatLayers; ++i)
    {
        if (mainFloatLayers[i].isVisible)
            gl.mixer.addSource(&gl.floatLayers[i].mapShader);
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

    int numLayers = static_cast<int>(mainFloatLayers.size());
    for (int l=0; l<numLayers; ++l)
    {
        //get the main and gpu layer
        MainFloatLayer& mainLayer = mainFloatLayers[l];
        GpuFloatLayer&  gpuLayer  = gl.floatLayers[l];
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



ColorMapper::GpuFloatLayer::
GpuFloatLayer(const SubRegion& region, ShaderDataSource* source, bool clamp) :
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
    mainColorLayers.resize(DATAMANAGER->getNumColorLayers());

    //create the new colorMaps with the current stamp to trigger their upload
    int demOffset = DATAMANAGER->hasDem() ? 1 : 0;
    int numLayers = DATAMANAGER->getNumLayerfLayers();
    numLayers    += demOffset;
    mainFloatLayers.resize(numLayers, MainFloatLayer(CURRENT_FRAME));

    //initialize all additional layers as transparent
    for (int l=0; l<numLayers; ++l)
        mainFloatLayers[l].mapColor = Misc::ColorMap::black;
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
