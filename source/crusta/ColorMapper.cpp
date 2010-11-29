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
    mapperConfigurationStamp(0), gpuLayersStamp(0), activeMapIndex(-1)
{
    //create temporary storage for colors
    GLColorMap::Color* colors =
        new GLColorMap::Color[SETTINGS->colorMapTexSize];

    //setup the default R, G and B maps
    float step  = 1.0f / float(SETTINGS->colorMapTexSize);
    GLColorMap::Color rgbStep[3] = {GLColorMap::Color(step, 0.0f, 0.0f, 0.0f),
                                    GLColorMap::Color(0.0f, step, 0.0f, 0.0f),
                                    GLColorMap::Color(0.0f, 0.0f, step, 0.0f)};

    for (int i=0; i<3; ++i)
    {
        GLColorMap::Color c(0.0f, 0.0f, 0.0f, 1.0f);
        for (int j=0; j<SETTINGS->colorMapTexSize; ++j)
        {
            colors[j] = c;
            for (int k=0; k<3; ++k)
                c[k] += rgbStep[i][k];
        }

        rgbMaps[i].setColors(SETTINGS->colorMapTexSize, colors);
        rgbMaps[i].setScalarRange(0.0, 255.0);
    }

    //setup the default transparent map
    GLColorMap::Color transparentColor(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i=0; i<SETTINGS->colorMapTexSize; ++i)
        colors[i] = transparentColor;
    transparentMap.setColors(SETTINGS->colorMapTexSize, colors);
    transparentMap.setScalarRange(0.0, 1.0);

    //clean-up temporary color storage
    delete[] colors;
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

GLColorMap* ColorMapper::
getColorMap(int mapIndex)
{
    assert(mapIndex>=0 && mapIndex<static_cast<int>(mainLayers.size()));
    return &mainLayers[mapIndex].mapColor;
}

int ColorMapper::
getActiveMap() const
{
    return activeMapIndex;
}

void ColorMapper::
setActiveMap(int mapIndex)
{
    assert(mapIndex>=0 && mapIndex<static_cast<int>(mainLayers.size()));
    activeMapIndex = mapIndex;
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
        gl.layers.push_back(GpuLayer(demData, &sources.height));
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
        gl.layers.push_back(GpuLayer(layerfData, &(*it)));
        RELEASE_PIN_BUFFER(gl.mapCache, index, layerfBuf);
    }

//- reconnect the mixer
    gl.mixer.clear();
    for (GpuLayers::iterator it=gl.layers.begin(); it!=gl.layers.end(); ++it)
        gl.mixer.addSource(&(it->mapShader));

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
            assert(mainLayer.mapColor.getColors() != NULL);
            gl.mapCache.stream(
                (SubRegion)gpuLayer.mapShader.getColorMapRegion(), GL_RGBA,
                GL_FLOAT, mainLayer.mapColor.getColors()[0].getRgba());
            CHECK_GLA
            gpuLayer.mapColorStamp = CURRENT_FRAME;
        }
        //update the range used in the shader if needed
        if (gpuLayer.mapRangeStamp < mainLayer.mapRangeStamp)
        {
            gpuLayer.mapShader.setScalarRange(
                mainLayer.mapColor.getScalarRangeMin(),
                mainLayer.mapColor.getScalarRangeMax());
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
GpuLayer(const SubRegion& region, ShaderDataSource* source) :
    mapShader("colorMapTex", region, source), mapColorStamp(0), mapRangeStamp(0)
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
    int demOffset       = DATAMANAGER->hasDem() ? 1 : 0;
    int numColorLayers  = DATAMANAGER->getNumColorLayers();
    int numLayers       = 3 * numColorLayers;
    numLayers          += DATAMANAGER->getNumLayerfLayers();
    numLayers          += demOffset;
    mainLayers.resize(numLayers, MainLayer(CURRENT_FRAME));

    //initialize the color map associated with elevation
    if (DATAMANAGER->hasDem())
        mainLayers[0].mapColor = transparentMap;

    //initialize the color maps associated with color inputs
    for (int l=0; l<numColorLayers; ++l)
    {
        for (int c=0; c<3; ++c)
            mainLayers[demOffset + 3*l + c].mapColor = rgbMaps[c];
    }

    //initialize the color maps associated with layerf inputs
    for (int l=demOffset+3*numColorLayers; l<numLayers; ++l)
        mainLayers[l].mapColor = transparentMap;
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
