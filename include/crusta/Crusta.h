#ifndef _Crusta_H_
#define _Crusta_H_

#include <string>
#include <vector>

#include <Threads/Mutex.h>

#include <crusta/basics.h>

class GLContextData;

BEGIN_CRUSTA

template <typename NodeDataType>
class CacheBuffer;

class DataManager;
class MapManager;
class QuadNodeMainData;
class QuadTerrain;

/** Main crusta class */
class Crusta
{
public:
    typedef std::vector<CacheBuffer<QuadNodeMainData>*> Actives;

    static void init(const std::string& demFileBase,
                     const std::string& colorFileBase);
    static void shutdown();

///\todo fix this API. potentially deprecate
    /** query the height of the surface closest to the query point */
    static double getHeight(double x, double y, double z);
    /** snap the given cartesian point to the surface of the terrain (at an
        optional offset) */
    static Point3 snapToSurface(const Point3& pos, Scalar offset=Scalar(0));

    static const FrameNumber& getCurrentFrame();
    static const FrameNumber& getLastScaleFrame();

    /** set the vertical exaggeration. Make sure to set this value within a
        frame callback so that it doesn't change during a rendering phase */
    static void setVerticalScale(double newVerticalScale);
    /** retrieve the vertical exaggeration factor */
    static double getVerticalScale();

    /** retrieve the data manager */
    static DataManager* getDataManager();
    /** retrieve the map manager */
    static MapManager* getMapManager();

    /** inform crusta of nodes that must be kept current */
    static void submitActives(const Actives& touched);

    static void frame();
    static void display(GLContextData& contextData);

protected:
    typedef std::vector<QuadTerrain*> RenderPatches;

    /** make sure the bounding objects used for visibility and LOD checks are
        up-to-date wrt to the vertical scale */
    static void confirmActives();

    /** keep track of the number of frames processed. Used, for example, by the
        cache to perform LRU that is aware of currently active nodes (the ones
        from the previous frame) */
    static FrameNumber currentFrame;
    /** keep track of the last frame at which the vertical scale was modified.
        The vertical scale affects the bounding primitives for the nodes and
        these must be updated each time the scale changes. Validity of a node's
        semi-static data can be verified by comparison with this number */
    static FrameNumber lastScaleFrame;

    /** the vertical scale to be applied to all surface elevations */
    static double verticalScale;

    /** the data management component */
    static DataManager* dataMan;
    /** the mapping management component */
    static MapManager* mapMan;

    /** the spheroid base patches used for rendering */
    static RenderPatches renderPatches;

    /** the nodes that have been touch during the traversals of the previous
        frame */
    static Actives actives;
    /** guarantee serial manipulation of the set of active nodes */
    static Threads::Mutex activesMutex;
};

END_CRUSTA

#endif //_Crusta_H_
