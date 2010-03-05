#ifndef _Crusta_H_
#define _Crusta_H_

#include <string>
#include <vector>

#include <GL/gl.h>
#include <GL/GLObject.h>
#include <Threads/Mutex.h>

#include <crusta/basics.h>
#include <crusta/LightingShader.h>

class GLContextData;

BEGIN_CRUSTA

template <typename NodeDataType>
class CacheBuffer;

class Cache;
class DataManager;
class MapManager;
class QuadNodeMainData;
class QuadTerrain;

/** Main crusta class */
class Crusta : public GLObject
{
public:
    typedef std::vector<CacheBuffer<QuadNodeMainData>*> Actives;

    void init(const std::string& demFileBase, const std::string& colorFileBase);
    void shutdown();

///\todo fix this API. potentially deprecate
    /** query the height of the surface closest to the query point */
    double getHeight(double x, double y, double z);
///\todo potentially deprecate
    /** snap the given cartesian point to the surface of the terrain (at an
        optional offset) */
    Point3 snapToSurface(const Point3& pos, Scalar offset=Scalar(0));
    /** intersect a ray with the crusta globe */
    HitResult intersect(const Ray& ray) const;

    const FrameNumber& getCurrentFrame()   const;
    const FrameNumber& getLastScaleFrame() const;

    /** configure the display of the terrain to use a texture or not */
    void useTexturedTerrain(bool useTex=true);
    /** set the vertical exaggeration. Make sure to set this value within a
        frame callback so that it doesn't change during a rendering phase */
    void setVerticalScale(double newVerticalScale);
    /** retrieve the vertical exaggeration factor */
    double getVerticalScale() const;

    /** map a 3D cartesian point specified wrt an unscaled globe representation
        to the corresponding point in a scaled representation */
    Point3 mapToScaledGlobe(const Point3& pos);
    /** map a 3D cartesian point specified on a scaled globe representation to
        the corresponding point in an unscaled representation */
    Point3 mapToUnscaledGlobe(const Point3& pos);

    Cache*       getCache()       const;
    DataManager* getDataManager() const;
    MapManager*  getMapManager()  const;

    LightingShader& getTerrainShader(GLContextData& contextData);

    /** inform crusta of nodes that must be kept current */
    void submitActives(const Actives& touched);

    void frame();
    void display(GLContextData& contextData);

protected:
    typedef std::vector<QuadTerrain*> RenderPatches;

    struct GlData : public GLObject::DataItem
    {
        GlData();
        ~GlData();

        GLuint frameBuf;
        GLuint colorBuf;
        GLuint terrainAttributesTex;
        GLuint depthStencilTex;

        LightingShader terrainShader;
    };

    /** make sure the bounding objects used for visibility and LOD checks are
        up-to-date wrt to the vertical scale */
    void confirmActives();

    /** prepare the offscreen framebuffer, i.e. copy current framebuffer */
    void prepareFrameBuffer(GlData* glData);
    /** commit the offscreen framebuffer to the default one */
    void commitFrameBuffer(GlData* glData);

    /** keep track of the number of frames processed. Used, for example, by the
        cache to perform LRU that is aware of currently active nodes (the ones
        from the previous frame) */
    FrameNumber currentFrame;
    /** keep track of the last frame at which the vertical scale was modified.
        The vertical scale affects the bounding primitives for the nodes and
        these must be updated each time the scale changes. Validity of a node's
        semi-static data can be verified by comparison with this number */
    FrameNumber lastScaleFrame;

    /** should the terrain rendering be using textures or not */
    bool isTexturedTerrain;
    /** the vertical scale to be applied to all surface elevations */
    Scalar verticalScale;
    /** the vertical scale that has been externally set. Buffers the scales
        changes up to the next frame call */
    Scalar newVerticalScale;

    /** the cache management component */
    Cache* cache;
    /** the data management component */
    DataManager* dataMan;
    /** the mapping management component */
    MapManager* mapMan;

    /** the spheroid base patches used for rendering */
    RenderPatches renderPatches;

    /** the nodes that have been touch during the traversals of the previous
        frame */
    Actives actives;
    /** guarantee serial manipulation of the set of active nodes */
    Threads::Mutex activesMutex;

    /** the global height range */
    Scalar globalElevationRange[2];

    /** the size of the current terrain attributes texture/buffer */
    int bufferSize[2];

//- inherited from GLObject
public:
   	virtual void initContext(GLContextData& contextData) const;
};

END_CRUSTA

#endif //_Crusta_H_
