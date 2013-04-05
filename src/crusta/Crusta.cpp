#include <crusta/Crusta.h>

#include <crusta/checkGl.h>
#include <crusta/ColorMapper.h>
#include <crusta/DataManager.h>
#include <crusta/Homography.h>
#include <crusta/map/MapManager.h>
#include <crusta/QuadCache.h>
#include <crusta/QuadTerrain.h>
#include <crusta/ResourceLocator.h>
#include <crustacore/Section.h>
#include <crusta/Sphere.h>
#include <crusta/SurfaceProbeTool.h>
#include <crusta/SliceTool.h>
#include <crusta/SurfaceTool.h>
#include <crusta/LayerToggleTool.h>
#include <crusta/Tool.h>
#include <crusta/Triangle.h>

#include <crusta/StatsManager.h>

#include <crusta/vrui.h>

///todo debug
#include <crusta/CrustaVisualizer.h>
#define CV(x) CrustaVisualizer::x


namespace crusta {

///\todo debug remove
bool PROJECTION_FAILED = false;

#if CRUSTA_ENABLE_DEBUG
int CRUSTA_DEBUG_LEVEL_MIN = CRUSTA_DEBUG_LEVEL_MIN_VALUE;
int CRUSTA_DEBUG_LEVEL_MAX = CRUSTA_DEBUG_LEVEL_MAX_VALUE;
#endif //CRUSTA_ENABLE_DEBUG

#if DEBUG_INTERSECT_CRAP
///\todo debug remove
bool DEBUG_INTERSECT = false;
#define DEBUG_INTERSECT_PEEK 0
#endif //DEBUG_INTERSECT_CRAP


/** the value 0 is used to indicate an invalid frame. Initialize the current
    stamp to be > 0 to allow the simple "< CURRENT_FRAME" tests to capture
    stamps generated during the initialization process for deferred GPU
    initializations */
FrameStamp CURRENT_FRAME(0.00000001);
FrameStamp LAST_FRAME(0.0);


#define CRUSTA_ENABLE_RECORD_FRAMERATE 0
#if CRUSTA_ENABLE_RECORD_FRAMERATE
class FrameRateRecorder
{
public:
    FrameRateRecorder(double iSampleStep) :
        sampleStep(iSampleStep), file("frames.txt")
    {
        schedule();
    }
    ~FrameRateRecorder()
    {
        file.close();
    }

protected:
    void schedule()
    {
        Misc::TimerEventScheduler* tes = Vrui::getTimerEventScheduler();
        double nextTickTime = tes->getCurrentTime() + sampleStep;
        tes->scheduleEvent(nextTickTime,this,&FrameRateRecorder::tickCallback);
    }

    void tickCallback(Misc::TimerEventScheduler::CallbackData*)
    {
        file << 1.0 / Vrui::getCurrentFrameTime() << ",";
        schedule();
    }

    double sampleStep;
    std::ofstream file;
};
FrameRateRecorder* CRUSTA_FRAMERATE_RECORDER;
#endif //CRUSTA_ENABLE_RECORD_FRAMERATE

CrustaGlData::
CrustaGlData() :
    gpuCache(NULL)
{
    glPushAttrib(GL_TEXTURE_BIT);

    glGenTextures(1, &symbolTex);
    glBindTexture(GL_TEXTURE_2D, symbolTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &colorMap);
    glBindTexture(GL_TEXTURE_1D, colorMap);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::string imgFileName = RESOURCELOCATOR.locateFile(
        "map/mapSymbolAtlas.tga");
    try
    {
        Misc::File imgFile(imgFileName.c_str(), "r");
        Images::TargaImageFileReader<Misc::File> imgFileReader(imgFile);
        Images::Image<uint8_t,4> img =
            imgFileReader.readImage<Images::Image<uint8_t,4> >();

        glBindTexture(GL_TEXTURE_2D, symbolTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.getWidth(), img.getHeight(),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixels());
    }
    catch (...)
    {
        float defaultTexel[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                     GL_RGBA, GL_FLOAT, defaultTexel);
    }
    CHECK_GLA

    glPopAttrib();
}

CrustaGlData::
~CrustaGlData()
{
    glDeleteTextures(1, &symbolTex);
    glDeleteTextures(1, &colorMap);
}



///\todo split crusta and planet
void Crusta::
init(const std::string& exePath, const Strings& settingsFiles,
     const std::string& resourcePath)
{
///\todo split crusta and planet
///\todo extend the interface to pass an optional configuration file

    //initialize the resource locator
    RESOURCELOCATOR.init(exePath, resourcePath);
		
		/* Initialize the current directory: */
		CURRENTDIRECTORY=Vrui::openDirectory(".");
		
    //initialize the crusta user settings
    SETTINGS = new CrustaSettings;
    SETTINGS->loadFromFiles(settingsFiles);

    //initialize the abstract crusta tool (adds an entry to the VRUI menu)
    Vrui::ToolFactory* crustaTool = Tool::init(NULL);
    //initialize the surface transformation tool
    SurfaceTool::init();
    //initialize the surface probing tool
    SurfaceProbeTool::init();
    //initialize the layer visibility toggle tool
    LayerToggleTool::init();

    if (SETTINGS->sliceToolEnable) {
        //initialize the slicing tool
        SliceTool::init();
    }

    /* start the frame counting at 2 because initialization code uses unsigneds
    that are initialized with 0. Thus if crustaFrameNumber starts at 0, the
    init code wouldn't be able to retrieve any cache buffers since all the
    buffers of the current and previous frame are locked */
    lastScaleStamp       = Math::Constants<double>::max;
    verticalScale        = 0.99999999999999;
    newVerticalScale     = 1.0;
    changedVerticalScale = 0.99999999999999;

///\todo separate crusta the application from a planet instance (current)
    CACHE       = new Cache;
    DATAMANAGER = new DataManager;
    COLORMAPPER = new ColorMapper;
///\todo VruiGlew dependent dynamic allocation
    QuadTerrain::initGlData();

    mapMan = new MapManager(crustaTool, this);
}

void Crusta::
shutdown()
{
    delete mapMan;
    for (RenderPatches::iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        delete *it;
    }

///\todo separate crusta the application from a planet instance (current)
    delete COLORMAPPER;
    delete DATAMANAGER;
    delete CACHE;
///\todo VruiGlew dependent dynamic allocation
    QuadTerrain::deleteGlData();

    delete SETTINGS;

#if CRUSTA_ENABLE_RECORD_FRAMERATE
delete CRUSTA_FRAMERATE_RECORDER;
#endif //CRUSTA_ENABLE_RECORD_FRAMERATE
}


void Crusta::
load(Strings& dataBases)
{
    //clear the currently loaded data
    unload();

    DATAMANAGER->load(dataBases);
    COLORMAPPER->load();

    globalElevationRange[0] =  Math::Constants<Scalar>::max;
    globalElevationRange[1] = -Math::Constants<Scalar>::max;

    const Polyhedron* const polyhedron = DATAMANAGER->getPolyhedron();

    size_t numPatches = polyhedron->getNumPatches();
    renderPatches.resize(numPatches);
    for (size_t i=0; i<numPatches; ++i)
    {
        renderPatches[i] = new QuadTerrain(i, polyhedron->getScope(i), this);
        const NodeData& root = *renderPatches[i]->getRootNode().node;
        globalElevationRange[0] = std::min(globalElevationRange[0],
                                           Scalar(root.elevationRange[0]));
        globalElevationRange[1] = std::max(globalElevationRange[1],
                                           Scalar(root.elevationRange[1]));
    }

    if (globalElevationRange[0]== Math::Constants<Scalar>::max ||
        globalElevationRange[1]==-Math::Constants<Scalar>::max)
    {
        globalElevationRange[0] = SETTINGS->terrainDefaultHeight;
        globalElevationRange[1] = SETTINGS->terrainDefaultHeight;
    }

    int heightMapIndex = COLORMAPPER->getHeightColorMapIndex();
    if (heightMapIndex >= 0)
    {
        Misc::ColorMap& colorMap = COLORMAPPER->getColorMap(heightMapIndex);
        colorMap.setValueRange(Misc::ColorMap::ValueRange(
            GLdouble(globalElevationRange[0]),
            GLdouble(globalElevationRange[1])));
    }

#if CRUSTA_ENABLE_DEBUG
debugTool = NULL;
#endif //CRUSTA_ENABLE_DEBUG
#if CRUSTA_ENABLE_RECORD_FRAMERATE
CRUSTA_FRAMERATE_RECORDER = new FrameRateRecorder(1.0/20.0);
#endif //CRUSTA_ENABLE_RECORD_FRAMERATE
}

void Crusta::
unload()
{
    //destroy all the current render patches
    for (RenderPatches::iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        delete *it;
    }
    renderPatches.clear();

    //destroy all the current maps
    mapMan->deleteAllShapes();

    //unload the data
    DATAMANAGER->unload();
    COLORMAPPER->unload();
}


SurfacePoint Crusta::
snapToSurface(const Geometry::Point<double,3>& pos, Scalar elevationOffset)
{
    SurfacePoint surfacePoint;
    if (renderPatches.empty())
        return surfacePoint;

    typedef NodeMainBuffer MainBuffer;
    typedef NodeMainData   MainData;

//- find the base patch
    MainData nodeData;
    for (RenderPatches::iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        //grab specification of the patch
        nodeData = (*it)->getRootNode();
        //check containment
        if (nodeData.node->scope.contains(pos))
            break;
        else
            nodeData.node = NULL;
    }

    assert(nodeData.node != NULL);

    NodeData* node = nodeData.node;
    assert(node->index.patch() < static_cast<size_t>(renderPatches.size()));

//- grab the finest level data possible
    const Geometry::Vector<double,3> vpos(pos);
    while (true)
    {
        //figure out the appropriate child by comparing against the mid-planes
        Geometry::Point<double,3> mid1, mid2;
        mid1 = Geometry::mid(node->scope.corners[0], node->scope.corners[1]);
        mid2 = Geometry::mid(node->scope.corners[2], node->scope.corners[3]);
        Geometry::Vector<double,3> vertical = Geometry::cross(Geometry::Vector<double,3>(mid1), Geometry::Vector<double,3>(mid2));
        vertical.normalize();

        mid1 = Geometry::mid(node->scope.corners[1], node->scope.corners[3]);
        mid2 = Geometry::mid(node->scope.corners[0], node->scope.corners[2]);
        Geometry::Vector<double,3> horizontal = Geometry::cross(Geometry::Vector<double,3>(mid1), Geometry::Vector<double,3>(mid2));
        horizontal.normalize();

        int childId = vpos*vertical   < 0 ? 0x1 : 0x0;
        childId    |= vpos*horizontal < 0 ? 0x2 : 0x0;

        //is it even possible to retrieve higher res data?
        if (!DATAMANAGER->existsChildData(nodeData))
            break;

        //try to grab the child for evaluation
        TreeIndex childIndex = node->index.down(childId);
        MainBuffer childBuf;
        bool childExists = DATAMANAGER->find(childIndex, childBuf);
        if (!childExists || !DATAMANAGER->isCurrent(childBuf))
        {
///\todo Vis2010 simplify. Don't allow loads of nodes from here
//            mainCache.request(MainCache::Request(0.0, nodeBuf, childId));
            break;
        }
        else
        {
            //switch to the child for traversal continuation
            nodeData = DATAMANAGER->getData(childBuf);
            node     = nodeData.node;
        }
    }

    //record the final node encountered
    surfacePoint.nodeIndex = node->index;

#if 0
    Geometry::Point<double,3> centroid = node->scope.getCentroid(node->scope.getRadius());
    Geometry::Vector<double,3> ups[4];
    for (int i=0; i<4; ++i)
    {
        ups[i] = Geometry::Vector<double,3>(node->scope.corners[i]);
        ups[i].normalize();
    }
    Geometry::Point<double,3> relativeCorners[4];
    for (int i=0; i<4; ++i)
    {
        for (int j=0; j<3; ++j)
            relativeCorners[i][j] = node->scope.corners[i][j] - centroid[j];
    }

//- compute projection matrix
    static const int tileRes = TILE_RESOLUTION;
    Homography toNormalized;
    //destinations are fll, flr, ful, bll, bur
    toNormalized.setDestination(Geometry::Point<double,3>(        0,         0, -1),
                                Geometry::Point<double,3>(tileRes-1,         0, -1),
                                Geometry::Point<double,3>(        0, tileRes-1, -1),
                                Geometry::Point<double,3>(        0,         0,  1),
                                Geometry::Point<double,3>(tileRes-1, tileRes-1,  1));
    //compute corresponding sources
    toNormalized.setSource(relativeCorners[0] - ups[0],
                           relativeCorners[1] - ups[1],
                           relativeCorners[2] - ups[2],
                           relativeCorners[0] + ups[0],
                           relativeCorners[3] + ups[3]);
    //compute the projective transformation
    toNormalized.computeProjective();

//- transform the test point
    Geometry::Point<double,3> relativePos(pos[0]-centroid[0],
                       pos[1]-centroid[1],
                       pos[2]-centroid[2]);

    typedef Geometry::HVector<Geometry::Point<double,3>::Scalar,Geometry::Point<double,3>::dimension> HPoint;

    const Homography::Projective& p = toNormalized.getProjective();

    Geometry::Point<double,3> projectedPos = p.transform(HPoint(relativePos)).toPoint();

//- determine the containing cell and the position within it
    Geometry::Point<int,2>& cellIndex    = surfacePoint.cellIndex;
    Geometry::Point<double,2>&  cellPosition = surfacePoint.cellPosition;
    cellIndex = Geometry::Point<int,2>(projectedPos[0], projectedPos[1]);
    for (int i=0; i<2; ++i)
    {
///\todo negative or greater than TILE_RESOLUTION-1 shouldn't happen
        if (cellIndex[i] < 0)
        {
            cellIndex[i]    = 0;
            cellPosition[i] = 0.0;
        }
        else if (cellIndex[i] >= tileRes-1)
        {
            cellIndex[i]    = tileRes-2;
            cellPosition[i] = 1.0;
        }
        else
        {
            cellPosition[i] = projectedPos[i] - cellIndex[i];
        }
    }

//- sample the cell
    int              linearOffset = cellIndex[1]*tileRes + cellIndex[0];
    Vertex*          cellV        = nodeData.geometry + linearOffset;
    DemHeight::Type* cellH        = nodeData.height   + linearOffset;

    const Vertex::Position* positions[4] = {
        &(cellV->position), &((cellV+1)->position),
        &((cellV+tileRes)->position), &((cellV+tileRes+1)->position) };

    DemHeight::Type heights[4] = {
        node->getHeight(*cellH),           node->getHeight(*(cellH+1)),
        node->getHeight(*(cellH+tileRes)), node->getHeight(*(cellH+tileRes+1))
    };
    //construct the properly extruded corners of the current cell
    Geometry::Vector<double,3> cellCorners[4];
    for (int i=0; i<4; ++i)
    {
        for (int j=0; j<3; ++j)
            cellCorners[i][j] = (*(positions[i]))[j] + node->centroid[j];
        Geometry::Vector<double,3> extrude(cellCorners[i]);
        extrude.normalize();
        extrude *= (heights[i] + elevationOffset) * getVerticalScale();
        cellCorners[i] += extrude;
    }
    //interpolate the corners
#if 1
///\todo get rid of all the renaming of the same things c0, v0...
//barycentric per triangle
    Geometry::Vector<double,2> v[3];
    Geometry::Vector<double,3> c[3];
    v[0] = Geometry::Vector<double,2>(0.0, 0.0);
    c[0] = cellCorners[0];
    if (cellPosition[0]>cellPosition[1])
    {
        v[1] = Geometry::Vector<double,2>(1.0, 0.0);
        c[1] = cellCorners[1];
        v[2] = Geometry::Vector<double,2>(1.0, 1.0);
        c[2] = cellCorners[3];
    }
    else
    {
        v[1] = Geometry::Vector<double,2>(1.0, 1.0);
        c[1] = cellCorners[3];
        v[2] = Geometry::Vector<double,2>(0.0, 1.0);
        c[2] = cellCorners[2];
    }
    //see http://en.wikipedia.org/wiki/Barycentric_coordinates_(mathematics)
    //compute the determinant
    static const double EPSILON = 0.000001;
    double det = (v[1][0]-v[0][0])*(v[2][1]-v[0][1]) - (v[2][0]-v[0][0])*(v[1][1]-v[0][1]);
    assert(Math::abs(det) > EPSILON);
    det = 1.0 / det;

    //compute weights
    double w[3];
    w[0] =(v[1][0]-cellPosition[0])*(v[2][1]-cellPosition[1])-(v[2][0]-cellPosition[0])*(v[1][1]-cellPosition[1]);
    w[0]*= det;
    assert(w[0]>=0.0 || w[0]<=1.0);
    w[1] =(v[2][0]-cellPosition[0])*(v[0][1]-cellPosition[1])-(v[0][0]-cellPosition[0])*(v[2][1]-cellPosition[1]);
    w[1]*= det;
    assert(w[1]>=0.0 || w[1]<=1.0);
    w[2] = 1.0 - w[0] - w[1];
    assert(w[2]>=0.0);

    surfacePoint.position = Geometry::Point<double,3>(w[0]*c[0] + w[1]*c[1] + w[2]*c[2]);
#else
//bi-linear
    Geometry::Vector<double,3> lower = cellCorners[0] +
                    cellPosition[0]*(cellCorners[1]-cellCorners[0]);
    Geometry::Vector<double,3> upper = cellCorners[2] +
                    cellPosition[0]*(cellCorners[3]-cellCorners[2]);
    surfacePoint.position = Geometry::Point<double,3>(lower + cellPosition[1]*(upper-lower));
#endif
#else
//- locate the cell of the refinement containing the point
    int numLevels = 1;
    int res = TILE_RESOLUTION-1;
    while (res > 1)
    {
        ++numLevels;
        res >>= 1;
    }

///\todo optimize the containement checks as above by using vert/horiz split
    Geometry::Point<int,2> offset(0.0, 0.0);
    Scope scope = node->scope;
    int shift   = (TILE_RESOLUTION-1) >> 1;
    for (int level=1; level<numLevels; ++level)
    {
        //compute the coverage for the children
        Scope childScopes[4];
        scope.split(childScopes);

        //find the child containing the point
        for (int i=0; i<4; ++i)
        {
            if (childScopes[i].contains(pos))
            {
                //adjust the current bottom-left offset and switch to that scope
                offset[0] += i&0x1 ? shift : 0;
                offset[1] += i&0x2 ? shift : 0;
                shift    >>= 1;
                scope      = childScopes[i];
                break;
            }
        }
    }

    //record the cell offset
    surfacePoint.cellIndex = offset;

///\todo record the proper cell position
    surfacePoint.cellPosition = Geometry::Point<double,2>(0.0, 0.0);

//- sample the cell
    static const int tileRes = TILE_RESOLUTION;
    int              linearOffset = offset[1]*tileRes + offset[0];
    Vertex*          cellV        = nodeData.geometry + linearOffset;
    DemHeight::Type* cellH        = nodeData.height   + linearOffset;

    const Vertex::Position* positions[4] = {
        &(cellV->position), &((cellV+1)->position),
        &((cellV+tileRes)->position), &((cellV+tileRes+1)->position) };

    DemHeight::Type heights[4] = {
        node->getHeight(*cellH),           node->getHeight(*(cellH+1)),
        node->getHeight(*(cellH+tileRes)), node->getHeight(*(cellH+tileRes+1))
    };
    //construct the corners of the current cell
    Geometry::Vector<double,3> cellCorners[4];
    for (int i=0; i<4; ++i)
    {
        for (int j=0; j<3; ++j)
        {
            cellCorners[i][j] = double((*(positions[i]))[j]) +
                                double(node->centroid[j]);
        }
        Geometry::Vector<double,3> extrude(cellCorners[i]);
        extrude.normalize();
        extrude *= (heights[i] + elevationOffset) * getVerticalScale();
        cellCorners[i] += extrude;
    }

    //intersect triangles of current cell
    Triangle t0(cellCorners[0], cellCorners[3], cellCorners[2]);
    Triangle t1(cellCorners[0], cellCorners[1], cellCorners[3]);

    Geometry::Ray<double,3> ray(pos, (-Geometry::Vector<double,3>(pos)).normalize());
    Geometry::HitResult<double> hit = t0.intersectRay(ray);
    if (!hit.isValid())
    {
        hit = t1.intersectRay(ray);
        if (!hit.isValid())
        {
///\todo this should not be possible!!!
CRUSTA_DEBUG(70, CRUSTA_DEBUG_OUT <<
"SnapToSurface failed triangle intersection test!\n";)
            Scalar height = nodeData.height[offset[1]*TILE_RESOLUTION +
                                            offset[0]];
            height        = node->getHeight(height) + elevationOffset;
            height       *= getVerticalScale();
            height       += SETTINGS->globeRadius ;

            Geometry::Vector<double,3> toPos = Geometry::Vector<double,3>(pos);
            toPos.normalize();
            toPos *= height;
            surfacePoint.position = Geometry::Point<double,3>(toPos[0], toPos[1], toPos[2]);
        }
        else
            surfacePoint.position = ray(hit.getParameter());
    }
    else
        surfacePoint.position = ray(hit.getParameter());
#endif

#if 0
std::cerr << "\n" << std::setprecision(std::numeric_limits<double>::digits10) <<
        "pos: " << pos[0] << " " << pos[1] << " " << pos[2] << "\n" <<
        "proj: " << projectedPos[0] << " " << projectedPos[1] << "\n" <<
        "sp.pos: " << surfacePoint.position[0] << " " << surfacePoint.position[1] << " " << surfacePoint.position[2] << "\n" <<
        "sp.node: " << surfacePoint.nodeIndex.med_str() << "\n" <<
        "sp.cellI: " << surfacePoint.cellIndex[0] << " " << surfacePoint.cellIndex[1] << "\n" <<
        "sp.cellP: " << surfacePoint.cellPosition[0] << " " << surfacePoint.cellPosition[1] << "\n";
#endif
    return surfacePoint;
}

SurfacePoint Crusta::
intersect(const Geometry::Ray<double,3>& ray) const
{
    if (renderPatches.empty())
        return SurfacePoint();

    const Scalar& verticalScale = getVerticalScale();

    Scalar gin, gout;
    //make sure the ray even intersects the outer shell of the globe
    Sphere shell(Geometry::Point<double,3>(0), SETTINGS->globeRadius +
                 verticalScale*globalElevationRange[1]);
    if (!shell.intersectRay(ray, gin, gout))
        return SurfacePoint();
    //don't use a starting point that is behind the origin
    gin = std::max(gin, 0.0);

    //try to constrain the exit point no further than the min-elevation shell
    double minShellIn, minShellOut;
    shell.setRadius(SETTINGS->globeRadius +
                    verticalScale*globalElevationRange[0]);
    if (shell.intersectRay(ray, minShellIn, minShellOut) && minShellIn>0.0)
        gout = minShellIn;

    //find the patch containing the entry point
    Geometry::Point<double,3> entry = ray(gin);
    const QuadTerrain* patch = NULL;
    NodeData*          node  = NULL;
    NodeMainData nodeData;
    for (RenderPatches::const_iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        nodeData = (*it)->getRootNode();
        node     = nodeData.node;
        if (node->scope.contains(entry))
        {
            patch = *it;
            break;
        }
    }

    assert(patch!=NULL && node!=NULL);

    //traverse terrain patches until intersection or ray exit
    SurfacePoint surfacePoint;
    Scalar tin           = gin;
    Scalar tout          = 0;
    int    sideIn        = -1;
    int    sideOut       = -1;
    int    mapSide[4][4] = {{2,3,0,1}, {1,2,3,0}, {0,1,2,3}, {3,0,1,2}};
    while (true)
    {
        surfacePoint = patch->intersect(ray, tin, sideIn, tout, sideOut, gout);
        if (surfacePoint.isValid())
            break;

/**\todo this is problematic because there are valence 5 vertices on the base
polyhedron (triacontahedron). The neighbor is not necessarily unique. This is
unlikely to be an issue because we are likely to intersect within the root.
Still this should be handled more robustly */
        //move to the patch on the exit side
        tin = tout;
        if (tin > gout)
            break;

        const Polyhedron* const polyhedron = DATAMANAGER->getPolyhedron();
        Polyhedron::Connectivity neighbors[4];
        polyhedron->getConnectivity(patch->getRootNode().node->index.patch(),
                                    neighbors);
        patch  = renderPatches[neighbors[sideOut][0]];
        sideIn = mapSide[neighbors[sideOut][1]][sideOut];
    }

    return surfacePoint;
}


void Crusta::
segmentCoverage(const Geometry::Point<double,3>& start, const Geometry::Point<double,3>& end,
                Shape::IntersectionFunctor& callback) const
{
    //explicitely check all the base render patches
    for (RenderPatches::const_iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        (*it)->segmentCoverage(start, end, callback);
    }
}


const FrameStamp& Crusta::
getLastScaleStamp() const
{
    return lastScaleStamp;
}

void Crusta::
setVerticalScale(double nVerticalScale)
{
    newVerticalScale = nVerticalScale;
}

double Crusta::
getVerticalScale() const
{
    return verticalScale;
}


Geometry::Point<double,3> Crusta::
mapToScaledGlobe(const Geometry::Point<double,3>& pos)
{
    Geometry::Vector<double,3> toPoint(pos[0], pos[1], pos[2]);
    Geometry::Vector<double,3> onSurface(toPoint);
    onSurface.normalize();
    onSurface *= SETTINGS->globeRadius;
    toPoint   -= onSurface;
    toPoint   *= verticalScale;
    toPoint   += onSurface;

    return Geometry::Point<double,3>(toPoint[0], toPoint[1], toPoint[2]);
}

Geometry::Point<double,3> Crusta::
mapToUnscaledGlobe(const Geometry::Point<double,3>& pos)
{
    Geometry::Vector<double,3> toPoint(pos[0], pos[1], pos[2]);
    Geometry::Vector<double,3> onSurface(toPoint);
    onSurface.normalize();
    onSurface *= SETTINGS->globeRadius;
    toPoint   -= onSurface;
    toPoint   /= verticalScale;
    toPoint   += onSurface;

    return Geometry::Point<double,3>(toPoint[0], toPoint[1], toPoint[2]);
}

MapManager* Crusta::
getMapManager() const
{
    return mapMan;
}


void Crusta::
frame()
{
///\todo split crusta and planet
    LAST_FRAME = CURRENT_FRAME;
    CURRENT_FRAME = Vrui::getApplicationTime();

///\todo hack. start the actual new frame
statsMan.newFrame();

#if 0
#if CRUSTA_ENABLE_DEBUG
if (debugTool!=NULL)
{
    bool  b0  = debugTool->getButton(0);
    bool& bl0 = debugTool->getButtonLast(0);
    if (b0 && b0!=bl0)
    {
        bl0 = b0;
        mapMan->load("/data/crusta/maps/azerbaijan/Crusta_Polylines.shp");
    }

    bool  b1  = debugTool->getButton(1);
    bool& bl1 = debugTool->getButtonLast(1);
    if (b1 && b1!=bl1)
    {
        bl1 = b1;
        SETTINGS->lineDecorated = true;
    }
}
#endif //CRUSTA_ENABLE_DEBUG
#endif

CRUSTA_DEBUG(8, CRUSTA_DEBUG_OUT <<
"\n\n\n--------------------------------------\n" << CURRENT_FRAME <<
"\n\n\n";)

    //apply the vertical scale changes
    if (verticalScale != changedVerticalScale)
    {
        verticalScale  = changedVerticalScale;
        lastScaleStamp = CURRENT_FRAME;
        mapMan->processVerticalScaleChange();
    }

    //check for scale changes since the last frame
    if (changedVerticalScale  != newVerticalScale)
    {
        //compute the translation the scale change implies on the navigation
        Geometry::Point<double,3> physicalCenter = Vrui::getDisplayCenter();
        const Vrui::NavTransform& navXform =
            Vrui::getNavigationTransformation();
        Geometry::Point<double,3> navCenter = navXform.inverseTransform(physicalCenter);

        Geometry::Vector<double,3> toCenter(navCenter[0], navCenter[1], navCenter[2]);
        Scalar height       = toCenter.mag();
        Scalar altitude     = (height - SETTINGS->globeRadius) / verticalScale;
        Scalar newHeight    = altitude*newVerticalScale + SETTINGS->globeRadius;
        Geometry::Vector<double,3> newToCenter = toCenter * (newHeight / height);
        Geometry::Vector<double,3> translation = toCenter - newToCenter;
        Vrui::setNavigationTransformation(navXform*
            Vrui::NavTransform::translate(translation));

        /* changes to the navigation only get applied in the next frame. Delay
           the processing to the frame that will have the proper navigation */
        changedVerticalScale = newVerticalScale;
    }

    //let the map manager update all the mapping stuff
    mapMan->frame();
}

struct DistanceToEyeSorter
{
    DistanceToEyeSorter(SurfaceApproximation* approx, const Geometry::Point<double,3>& eye) :
        surface(approx), eyePosition(eye)
    {}
    void sortVisibles()
    {
        std::sort(surface->visibles.begin(), surface->visibles.end(), *this);
    }

    bool operator()(int i, int j)
    {
        double distI = Geometry::dist(eyePosition,
                                      surface->nodes[i].node->boundingCenter);
        double distJ = Geometry::dist(eyePosition,
                                      surface->nodes[j].node->boundingCenter);
        return distI > distJ;
    }
    SurfaceApproximation* surface;
    Geometry::Point<double,3> eyePosition;
};

void Crusta::
display(GLContextData& contextData)
{
    CHECK_GLA

    CACHE->display(contextData);
    CHECK_GLA
    DATAMANAGER->display(contextData);
    CHECK_GLA
    COLORMAPPER->configureShaders(contextData);
    CHECK_GLA

    CrustaGlData* glData = contextData.retrieveDataItem<CrustaGlData>(this);
    glData->gpuCache = &CACHE->getGpuCache(contextData);

//- prepare the surface approximation and renderable representation
    SurfaceApproximation surface;

    //generate the terrain representation
    for (RenderPatches::const_iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        (*it)->prepareDisplay(contextData, surface);
        CHECK_GLA
    }

    //sort the visible tiles with respect to the distance to the camera
    Geometry::Point<double,3> eyePosition =
        Vrui::getDisplayState(contextData).viewer->getHeadPosition();
    eyePosition =
        Vrui::getInverseNavigationTransformation().transform(eyePosition);
    DistanceToEyeSorter sorter(&surface, eyePosition);
    sorter.sortVisibles();

statsMan.extractTileStats(surface);

CRUSTA_DEBUG(9, CRUSTA_DEBUG_OUT <<
"Number of nodes to render: " << surface.visibles.size() << "\n";)

    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    glPushAttrib(GL_TEXTURE_BIT);

    //update the map data
///\todo integrate properly (VIS 2010)
    if (SETTINGS->lineDecorated)
    {
        mapMan->updateLineData(surface);
        CHECK_GLA
    }

//- draw the current terrain and map data
    //bind the colormap texture
    glActiveTexture(GL_TEXTURE6);
    COLORMAPPER->bindColorMaps(contextData);

    //have the QuadTerrain draw the surface approximation
    CHECK_GLA

    //draw the terrain
    glData->terrainShader.setLinesDecorated(SETTINGS->lineDecorated);
    glData->terrainShader.update(contextData);
    glData->terrainShader.enable();

    COLORMAPPER->updateShaders(contextData);

    glData->terrainShader.setVerticalScale(getVerticalScale());
    glData->terrainShader.setTextureStep(TILE_TEXTURE_COORD_STEP);
    glData->terrainShader.setColorNodata(DATAMANAGER->getColorNodata());
    glData->terrainShader.setLayerfNodata(DATAMANAGER->getLayerfNodata());
    glData->terrainShader.setDemDefault(SETTINGS->terrainDefaultHeight);

///\todo this needs to be tweakable
    if (SETTINGS->lineDecorated)
    {
        float scaleFac = Vrui::getNavigationTransformation().getScaling();
        ShaderDecoratedLineRenderer& decorated =
            glData->terrainShader.getDecoratedLineRenderer();
        decorated.setSymbolLength(SETTINGS->lineSymbolLength * scaleFac);
        decorated.setSymbolWidth(SETTINGS->lineSymbolWidth / scaleFac);
    }
    CHECK_GLA

    QuadTerrain::display(contextData, glData, surface);
    CHECK_GLA

    glData->terrainShader.disable();
    CHECK_GLA

    //let the map manager draw all the mapping stuff
    mapMan->display(contextData, surface);
    CHECK_GLA

    glPopAttrib();
    glActiveTexture(activeTexture);
}


void Crusta::
setDecoratedVectorArt(bool flag)
{
    SETTINGS->lineDecorated = flag;
}

void Crusta::
setTerrainSpecularColor(const Color& color)
{
    SETTINGS->terrainSpecularColor = color;
}

void Crusta::
setTerrainShininess(const float& shininess)
{
    SETTINGS->terrainShininess = shininess;
}


void Crusta::
initContext(GLContextData& contextData) const
{
    CrustaGlData* glData   = new CrustaGlData();
    contextData.addDataItem(this, glData);
}



void Crusta::
confirmLineCoverageRemoval(Shape* shape, Shape::ControlPointHandle cp)
{
    for (RenderPatches::const_iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        const NodeMainData& root = (*it)->getRootNode();
        (*it)->confirmLineCoverageRemoval(root, shape, cp);
    }
}

void Crusta::
validateLineCoverage()
{
    for (RenderPatches::const_iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        const NodeMainData& root = (*it)->getRootNode();
        (*it)->validateLineCoverage(root);
    }
}


} //namespace crusta
