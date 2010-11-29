#ifndef _ColorMapper_H_
#define _ColorMapper_H_


#include <vector>

#include <GL/VruiGlew.h> //must include glew before any GL
#include <GL/GLColorMap.h>
#include <GL/GLObject.h>

#include <crusta/QuadCache.h>
#include <crusta/shader/ShaderColorMapper.h>
#include <crusta/shader/ShaderColorMixer.h>
#include <crusta/shader/ShaderColorReader.h>


BEGIN_CRUSTA


class ColorMapper : public GLObject
{
public:
    struct MainLayer
    {
        MainLayer(FrameStamp initialStamp=0);
        /** main memory representation for the color map. Used to interact with
         the UI */
        GLColorMap mapColor;
        /** stamp for the age of the colors of the map */
        FrameStamp mapColorStamp;
        /** stamp for the range of the map */
        FrameStamp mapRangeStamp;
    };
    typedef std::vector<MainLayer> MainLayers;

    ColorMapper();

    void load();
    void unload();

    void touchColor(int mapIndex);
    void touchRange(int mapIndex);

    FrameStamp getMapperConfigurationStamp() const;

    int getNumColorMaps() const;
    int getHeightColorMapIndex() const;
    GLColorMap* getColorMap(int mapIndex);

///\todo eurovis 2011 does the active index stuff make sense here
    int getActiveMap() const;
    void setActiveMap(int mapIndex);

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

    struct GpuLayer
    {
        //use a regular SubRegion because we need a range stamp anyway
        GpuLayer(const SubRegion& region, ShaderDataSource* source);

        /** stores the sub region of the color maps explicitly. Maps cannot
            be dynamically reassigned/swaped, thus we can keep the subregions
            directly in the shader*/
        ShaderColorMapper mapShader;
        FrameStamp        mapColorStamp;
        FrameStamp        mapRangeStamp;
    };
    typedef std::vector<GpuLayer> GpuLayers;

    struct GlItem : public GLObject::DataItem
    {
        GlItem();

        /** main storage for the color maps in GPU memory */
        GpuColorMapCache mapCache;
        /** the color mixer shader */
        ShaderColorMixer mixer;
        /** reader for color layers */
        ShaderColorReaders colors;
        /** individual layer components */
        GpuLayers layers;
        /** stamp used to update the GPU layers */
        FrameStamp layersStamp;
    };

    /** generate the main memory layer representation */
    void configureMainLayers();

    /** individual main memory layer components */
    MainLayers mainLayers;

    /** stamp to export the age of the managed configuration */
    FrameStamp mapperConfigurationStamp;
    /** stamp used to flag the update of the GPU layers */
    FrameStamp gpuLayersStamp;

    /** the default transparent map */
    GLColorMap transparentMap;

///\todo eurovis 2011 does the active index stuff make sense here
    int activeMapIndex;

//- inherited from GLObject
public:
    virtual void initContext(GLContextData& contextData) const;
};


extern ColorMapper* COLORMAPPER;

END_CRUSTA


#endif //_ColorMapper_H_
