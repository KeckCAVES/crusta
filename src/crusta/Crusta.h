#ifndef _Crusta_H_
#define _Crusta_H_

#include <string>
#include <vector>

#include <crusta/GL/VruiGlew.h>
#include <GL/GLObject.h>
#include <Threads/Mutex.h>

#include <crustacore/basics.h>
#include <crusta/glbasics.h>
#include <crusta/CrustaSettings.h>
#include <crusta/LightingShader.h>
#include <crusta/map/Shape.h>
#include <crusta/QuadCache.h>
#include <crusta/SurfacePoint.h>

#if CRUSTA_ENABLE_DEBUG
#include <crusta/DebugTool.h>
#include <crusta/Timer.h>
#endif //CRUSTA_ENABLE_DEBUG


class GLColorMap;
class GLContextData;

BEGIN_CRUSTA

template <typename NodeDataType>
class CacheBuffer;

class MapManager;
class NodeData;
class QuadTerrain;

struct CrustaGlData : public GLObject::DataItem
{
    CrustaGlData();
    ~CrustaGlData();

///\todo it should be possible to remove this here
    /** store a handle to the gpu cache of this context for convenient
        access */
    GpuCache* gpuCache;

    /** texture object for the atlas containing the visual symbol
        reprepsentations */
    GLuint symbolTex;
    /** texture object for the elevation color ramp */
    GLuint colorMap;

    /** the surface rendering shader */
    LightingShader terrainShader;
};

///\todo separate crusta the application from a planet instance (current)
/** Main crusta class */
class Crusta : public GLObject
{
public:
    typedef std::vector<std::string> Strings;

    void init(const std::string& exePath, const Strings& settingsFiles,
              const std::string& resourcePath);
    void shutdown();

    void load(Strings& dataBases);
    void unload();

///\todo potentially deprecate
    /** snap the given cartesian point to the surface of the terrain (at an
        optional offset) */
    SurfacePoint snapToSurface(const Geometry::Point<double,3>& pos, Scalar offset=Scalar(0));
    /** intersect a ray with the crusta globe */
    SurfacePoint intersect(const Geometry::Ray<double,3>& ray) const;

    /** determine the coverage of a single segment with the global hierarchy */
    void segmentCoverage(const Geometry::Point<double,3>& start, const Geometry::Point<double,3>& end,
                   Shape::IntersectionFunctor& callback) const;

    const FrameStamp& getLastScaleStamp() const;

    /** set the vertical exaggeration. Make sure to set this value within a
        frame callback so that it doesn't change during a rendering phase */
    void setVerticalScale(double newVerticalScale);
    /** retrieve the vertical exaggeration factor */
    double getVerticalScale() const;

    /** map a 3D cartesian point specified wrt an unscaled globe representation
        to the corresponding point in a scaled representation */
    Geometry::Point<double,3> mapToScaledGlobe(const Geometry::Point<double,3>& pos);
    /** map a 3D cartesian point specified on a scaled globe representation to
        the corresponding point in an unscaled representation */
    Geometry::Point<double,3> mapToUnscaledGlobe(const Geometry::Point<double,3>& pos);

    MapManager* getMapManager()  const;

    void frame();
    void display(GLContextData& contextData);

    /** toggle the decoration of the line mapping */
    void setDecoratedVectorArt(bool flag=true);
    /** change the specular color of the terrain surface */
    void setTerrainSpecularColor(const Color& color);
    /** change the shininess of the terrain surface */
    void setTerrainShininess(const float& shininess);


#if CRUSTA_ENABLE_DEBUG
DebugTool* debugTool;
Timer      debugTimers[10];
#endif //CRUSTA_ENABLE_DEBUG

///\todo debug
void confirmLineCoverageRemoval(Shape* shape, Shape::ControlPointHandle cp);
void validateLineCoverage();

protected:
    typedef std::vector<QuadTerrain*> RenderPatches;

    /** keep track of the last stamp at which the vertical scale was modified.
        The vertical scale affects the bounding primitives for the nodes and
        these must be updated each time the scale changes. Validity of a node's
        semi-static data can be verified by comparison with this number */
    FrameStamp lastScaleStamp;

    /** the vertical scale to be applied to all surface elevations */
    Scalar verticalScale;
    /** the vertical scale that has been externally set */
    Scalar newVerticalScale;
    /** buffers the scales changes up to the next frame call to maintain
        consistency with the change of the navigation transformation */
    Scalar changedVerticalScale;

    /** the mapping management component */
    MapManager* mapMan;

    /** the spheroid base patches used for rendering */
    RenderPatches renderPatches;

    /** the global height range */
    Scalar globalElevationRange[2];

//- inherited from GLObject
public:
    virtual void initContext(GLContextData& contextData) const;
};

END_CRUSTA

#endif //_Crusta_H_
