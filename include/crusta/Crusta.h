#ifndef _Crusta_H_
#define _Crusta_H_

#include <string>
#include <vector>

#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GL/GLShader.h>
#include <Threads/Mutex.h>

#include <crusta/basics.h>
#include <crusta/CrustaSettings.h>
#include <crusta/LightingShader.h>
#include <crusta/QuadCache.h>
#include <crusta/map/Shape.h>

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
    void init(const std::string& demFileBase, const std::string& colorFileBase,
              const std::string& settingsFile);
    void shutdown();

///\todo potentially deprecate
    /** snap the given cartesian point to the surface of the terrain (at an
        optional offset) */
    Point3 snapToSurface(const Point3& pos, Scalar offset=Scalar(0));
    /** intersect a ray with the crusta globe */
    HitResult intersect(const Ray& ray) const;

    /** intersect a single segment with the global hierarchy */
    void intersect(Shape::ControlPointHandle start,
                   Shape::IntersectionFunctor& callback) const;

    const FrameStamp& getLastScaleStamp() const;

    /** configure the display of the terrain to use a texture or not */
    void setTexturingMode(int mode);
    /** set the vertical exaggeration. Make sure to set this value within a
        frame callback so that it doesn't change during a rendering phase */
    void setVerticalScale(double newVerticalScale);
    /** retrieve the vertical exaggeration factor */
    double getVerticalScale() const;

    /** query the color map for external update */
    GLColorMap* getColorMap();
    /** signal crusta that changes have been made to the color map */
    void touchColorMap();
    /** upload the color map to the GL */
    void uploadColorMap(GLuint colorTex);

    /** map a 3D cartesian point specified wrt an unscaled globe representation
        to the corresponding point in a scaled representation */
    Point3 mapToScaledGlobe(const Point3& pos);
    /** map a 3D cartesian point specified on a scaled globe representation to
        the corresponding point in an unscaled representation */
    Point3 mapToUnscaledGlobe(const Point3& pos);

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

    /** texturing mode to use for terrain rendering */
    int texturingMode;
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

    /** flags if the color map has been changed and needs to be updated to the
        GL */
    bool colorMapDirty;
    /** the color map used to color the elevation of the terrain */
    GLColorMap* colorMap;

//- inherited from GLObject
public:
    virtual void initContext(GLContextData& contextData) const;
};

END_CRUSTA

#endif //_Crusta_H_
