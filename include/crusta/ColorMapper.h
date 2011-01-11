#ifndef _ColorMapper_H_
#define _ColorMapper_H_


#include <vector>

#include <GL/VruiGlew.h> //must include glew before any GL
#include <GL/GLColorMap.h>
#include <GL/GLObject.h>
#include <Misc/ColorMap.h>

#include <crusta/QuadCache.h>
#include <crusta/shader/ShaderColorMapper.h>
#include <crusta/shader/ShaderColorMixer.h>
#include <crusta/shader/ShaderColorMultiplier.h>
#include <crusta/shader/ShaderColorReader.h>


BEGIN_CRUSTA

/**\todo separate color mapper from a layer manager */
class ColorMapper : public GLObject
{
public:
    typedef std::vector<std::string> Strings;

    struct MainLayer
    {
        MainLayer();
        /** flag used to determine the visibility of the layer */
        bool isVisible;
    };
    typedef std::vector<MainLayer> MainLayers;

    struct MainFloatLayer : public MainLayer
    {
        MainFloatLayer(FrameStamp initialStamp=0);
        /** flag used to determine if the color mapping is clamped */
        bool isClamped;
        /** main memory representation for the color map. Used to interact with
            the UI */
        Misc::ColorMap mapColor;
        /** stamp for the age of the colors of the map */
        FrameStamp mapColorStamp;
        /** stamp for the range of the map */
        FrameStamp mapRangeStamp;
    };
    typedef std::vector<MainFloatLayer> MainFloatLayers;

    ColorMapper();

    void load();
    void unload();

    void touchColor(int layerIndex);
    void touchRange(int layerIndex);

    void setVisible(int layerIndex, bool visible);
    bool isVisible(int layerIndex) const;
    void setClamping(int layerIndex, bool clamp);
    bool isClamped(int layerIndex) const;

    FrameStamp getMapperConfigurationStamp() const;

    Strings getLayerNames() const;

    int getNumColorMaps() const;
    int getHeightColorMapIndex() const;
    Misc::ColorMap& getColorMap(int layerIndex);
    const Misc::ColorMap& getColorMap(int layerIndex) const;

    bool isColorLayer(int layerIndex) const;
    bool isFloatLayer(int layerIndex) const;

    int getActiveLayer() const;
    void setActiveLayer(int layerIndex);

    /** retrieve the color mixer that provides the final color of the mapper */
    ShaderDataSource* getColorSource(GLContextData& contextData);

    /** generate the GPU layer representation */
    void configureShaders(GLContextData& contextData);
    /** update the GPU color maps */
    void updateShaders(GLContextData& contextData);
    void bindColorMaps(GLContextData& contextData);

protected:
    typedef Gpu1dAtlasCache<SubRegionBuffer> GpuColorMapCache;
    typedef std::vector<ShaderColorReader>   ShaderColorReaders;

    struct GpuFloatLayer
    {
        //use a regular SubRegion because we need a range stamp anyway
        GpuFloatLayer(const SubRegion& region, ShaderDataSource* source,
                      bool clamp);

        /** stores the sub region of the color maps explicitly. Maps cannot
            be dynamically reassigned/swaped, thus we can keep the subregions
            directly in the shader*/
        ShaderColorMapper mapShader;
        FrameStamp        mapColorStamp;
        FrameStamp        mapRangeStamp;
    };
    typedef std::vector<GpuFloatLayer> GpuFloatLayers;

    struct GlItem : public GLObject::DataItem
    {
        GlItem();

        /** main storage for the color maps in GPU memory */
        GpuColorMapCache mapCache;
        /** the color mixer shader */
        ShaderColorMixer mixer;
/** the color multiplier shader */
ShaderColorMultiplier multiplier;
        /** reader for color layers */
        ShaderColorReaders colors;
        /** individual layer components */
        GpuFloatLayers floatLayers;
        /** stamp used to update the GPU layers */
        FrameStamp layersStamp;
    };

    /** generate the main memory layer representation */
    void configureMainLayers();

    /** individual main memory color layer components */
    MainLayers mainColorLayers;
    /** individual main memory float layer components */
    MainFloatLayers mainFloatLayers;

    /** stamp to export the age of the managed configuration */
    FrameStamp mapperConfigurationStamp;
    /** stamp used to flag the update of the GPU layers */
    FrameStamp gpuLayersStamp;

    /** stores the index of the currently active layer */
    int activeLayerIndex;
//- inherited from GLObject
public:
    virtual void initContext(GLContextData& contextData) const;
};


extern ColorMapper* COLORMAPPER;

END_CRUSTA


#endif //_ColorMapper_H_
