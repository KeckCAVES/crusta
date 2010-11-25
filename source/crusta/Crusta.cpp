#include <crusta/Crusta.h>

#include <Images/Image.h>
#include <Images/TargaImageFileReader.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLColorMap.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Misc/File.h>
#include <Misc/ThrowStdErr.h>
#include <Vrui/Vrui.h>

#include <crusta/checkGl.h>
#include <crusta/DataManager.h>
#include <crusta/ElevationRangeTool.h>
#include <crusta/map/MapManager.h>
#include <crusta/QuadCache.h>
#include <crusta/QuadTerrain.h>
#include <crusta/Section.h>
#include <crusta/Sphere.h>
#include <crusta/SurfaceTool.h>
#include <crusta/Tool.h>
#include <crusta/Triangle.h>

#include <crusta/StatsManager.h>

///todo debug
#include <crusta/CrustaVisualizer.h>
#define CV(x) CrustaVisualizer::x


BEGIN_CRUSTA

///\todo debug remove
bool PROJECTION_FAILED = false;


#if CRUSTA_ENABLE_DEBUG
int CRUSTA_DEBUG_LEVEL_MIN = 100;
int CRUSTA_DEBUG_LEVEL_MAX = 100;
#endif //CRUSTA_ENABLE_DEBUG

#if DEBUG_INTERSECT_CRAP
///\todo debug remove
bool DEBUG_INTERSECT = false;
#define DEBUG_INTERSECT_PEEK 0
#endif //DEBUG_INTERSECT_CRAP


FrameStamp CURRENT_FRAME(0);


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

    std::string imgFileName(CRUSTA_SHARE_PATH);
    imgFileName += "/mapSymbolAtlas.tga";
    try
    {
        Misc::File imgFile(imgFileName.c_str(), "r");
        Images::TargaImageFileReader<Misc::File> imgFileReader(imgFile);
        Images::Image<uint8,4> img =
            imgFileReader.readImage<Images::Image<uint8,4> >();

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
init(const std::vector<std::string>& settingsFiles)
{
///\todo split crusta and planet
///\todo extend the interface to pass an optional configuration file
    //initialize the crusta user settings
    SETTINGS = new CrustaSettings;
    SETTINGS->loadFromFiles(settingsFiles);

    //initialize the surface transformation tool
    SurfaceTool::init();
    //initialize the abstract crusta tool (adds an entry to the VRUI menu)
    Vrui::ToolFactory* crustaTool = Tool::init(NULL);

    /* start the frame counting at 2 because initialization code uses unsigneds
    that are initialized with 0. Thus if crustaFrameNumber starts at 0, the
    init code wouldn't be able to retrieve any cache buffers since all the
    buffers of the current and previous frame are locked */
    lastScaleStamp       = Math::Constants<double>::max;
    texturingMode        = 2;
    verticalScale        = 0.99999999999999;
    newVerticalScale     = 1.0;
    changedVerticalScale = 0.99999999999999;

///\todo separate crusta the application from a planet instance (current)
    CACHE       = new Cache;
    DATAMANAGER = new DataManager;
///\todo VruiGlew dependent dynamic allocation
    QuadTerrain::initGlData();

    mapMan = new MapManager(crustaTool, this);

    ElevationRangeTool::init(crustaTool);

    globalElevationRange[0] = 0;
    globalElevationRange[1] = 0;

    colorMapDirty = false;
    static const int numColorMapEntries = 1024;
    GLColorMap::Color dummyColorMap[numColorMapEntries];
    colorMap = new GLColorMap(numColorMapEntries, dummyColorMap,
                              GLdouble(globalElevationRange[0]),
                              GLdouble(globalElevationRange[1]));
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

    globalElevationRange[0] =  Math::Constants<Scalar>::max;
    globalElevationRange[1] = -Math::Constants<Scalar>::max;

    const Polyhedron* const polyhedron = DATAMANAGER->getPolyhedron();

    uint numPatches = polyhedron->getNumPatches();
    renderPatches.resize(numPatches);
    for (uint i=0; i<numPatches; ++i)
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

    colorMapDirty = true;
    static const int numColorMapEntries = 1024;
    GLColorMap::Color dummyColorMap[numColorMapEntries];
    colorMap = new GLColorMap(numColorMapEntries, dummyColorMap,
                              GLdouble(globalElevationRange[0]),
                              GLdouble(globalElevationRange[1]));

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
}


Point3 Crusta::
snapToSurface(const Point3& pos, Scalar elevationOffset)
{
    if (renderPatches.empty())
        return pos;

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
    assert(node->index.patch < static_cast<uint>(renderPatches.size()));

//- grab the finest level data possible
    const Vector3 vpos(pos);
    while (true)
    {
        //figure out the appropriate child by comparing against the mid-planes
        Point3 mid1, mid2;
        mid1 = Geometry::mid(node->scope.corners[0], node->scope.corners[1]);
        mid2 = Geometry::mid(node->scope.corners[2], node->scope.corners[3]);
        Vector3 vertical = Geometry::cross(Vector3(mid1), Vector3(mid2));
        vertical.normalize();

        mid1 = Geometry::mid(node->scope.corners[1], node->scope.corners[3]);
        mid2 = Geometry::mid(node->scope.corners[0], node->scope.corners[2]);
        Vector3 horizontal = Geometry::cross(Vector3(mid1), Vector3(mid2));
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
        if (!childExists)
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

//- locate the cell of the refinement containing the point
    int numLevels = 1;
    int res = TILE_RESOLUTION-1;
    while (res > 1)
    {
        ++numLevels;
        res >>= 1;
    }

///\todo optimize the containement checks as above by using vert/horiz split
    Scope scope   = node->scope;
    int offset[2] = { 0, 0 };
    int shift     = (TILE_RESOLUTION-1) >> 1;
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
    Vector3 cellCorners[4];
    for (int i=0; i<4; ++i)
    {
        for (int j=0; j<3; ++j)
            cellCorners[i][j] = (*(positions[i]))[j] + node->centroid[j];
        Vector3 extrude(cellCorners[i]);
        extrude.normalize();
        extrude *= (heights[i] + elevationOffset) * getVerticalScale();
        cellCorners[i] += extrude;
    }

    //intersect triangles of current cell
    Triangle t0(cellCorners[0], cellCorners[3], cellCorners[2]);
    Triangle t1(cellCorners[0], cellCorners[1], cellCorners[3]);

    Ray ray(pos, (-Vector3(pos)).normalize());
    HitResult hit = t0.intersectRay(ray);
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

            Vector3 toPos = Vector3(pos);
            toPos.normalize();
            toPos *= height;
            return Point3(toPos[0], toPos[1], toPos[2]);
        }
    }

    return ray(hit.getParameter());
}

HitResult Crusta::
intersect(const Ray& ray) const
{
#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
CrustaVisualizer::clearAll();
CrustaVisualizer::addRay(ray, 0);
#if DEBUG_INTERSECT_PEEK
CrustaVisualizer::peek();
#endif //DEBUG_INTERSECT_PEEK
//CrustaVisualizer::show("Ray");
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP

    if (renderPatches.empty())
        return HitResult();

    const Scalar& verticalScale = getVerticalScale();

    //intersect the ray with the global outer shells to determine starting point
    Sphere shell(Point3(0), SETTINGS->globeRadius +
                 verticalScale*globalElevationRange[1]);
    Scalar gin, gout;
    bool intersects = shell.intersectRay(ray, gin, gout);
    gin = std::max(gin, 0.0);

    if (!intersects)
        return HitResult();

    shell.setRadius(SETTINGS->globeRadius +
                    verticalScale*globalElevationRange[0]);
    HitResult hit = shell.intersectRay(ray);
    if (hit.isValid())
        gout = hit.getParameter();

#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
CrustaVisualizer::addHit(ray, HitResult(gin), 8);
#if DEBUG_INTERSECT_PEEK
CrustaVisualizer::peek();
#endif //DEBUG_INTERSECT_PEEK
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP

    //find the patch containing the entry point
    Point3 entry = ray(gin);
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

#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
{
Point3s verts;
verts.resize(2);
verts[0] = ray(gin);
verts[1] = ray(gout);
CrustaVisualizer::addPrimitive(GL_POINTS, verts, 9, Color(0.2, 0.1, 0.9, 1.0));
#if DEBUG_INTERSECT_PEEK
CrustaVisualizer::peek();
#endif //DEBUG_INTERSECT_PEEK
}
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP

    //traverse terrain patches until intersection or ray exit
    Scalar tin           = gin;
    Scalar tout          = 0;
    int    sideIn        = -1;
    int    sideOut       = -1;
    int    mapSide[4][4] = {{2,3,0,1}, {1,2,3,0}, {0,1,2,3}, {3,0,1,2}};
#if DEBUG_INTERSECT_CRAP
int patchesVisited = 0;
#endif //DEBUG_INTERSECT_CRAP
    while (true)
    {
        hit = patch->intersect(ray, tin, sideIn, tout, sideOut, gout);
        if (hit.isValid())
            break;

        //move to the patch on the exit side
        tin = tout;
        if (tin > gout)
            break;

#if DEBUG_INTERSECT_CRAP
const QuadTerrain* oldPatch = patch;
#endif //DEBUG_INTERSECT_CRAP

        const Polyhedron* const polyhedron = DATAMANAGER->getPolyhedron();
        Polyhedron::Connectivity neighbors[4];
        polyhedron->getConnectivity(patch->getRootNode().node->index.patch,
                                    neighbors);
        patch  = renderPatches[neighbors[sideOut][0]];
        sideIn = mapSide[neighbors[sideOut][1]][sideOut];

#if DEBUG_INTERSECT_CRAP
Scalar E = 0.00001;
int sides[4][2] = {{3,2}, {2,0}, {0,1}, {1,3}};
const Scope& oldS = oldPatch->getRootNode().scope;
const Scope& newS = patch->getRootNode().scope;
assert(Geometry::dist(oldS.corners[sides[sideOut][0]], newS.corners[sides[sideIn][1]])<E);
assert(Geometry::dist(oldS.corners[sides[sideOut][1]], newS.corners[sides[sideIn][0]])<E);
std::cerr << "visited: " << ++patchesVisited << std::endl;
#endif //DEBUG_INTERSECT_CRAP
    }

    return hit;
}


void Crusta::
intersect(Shape::ControlPointHandle start,
          Shape::IntersectionFunctor& callback) const
{
    //find the patch containing the entry point
    const Point3&      entry = start->pos;
    const QuadTerrain* patch = NULL;
    const NodeData*    node  = NULL;
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
    Ray ray(start->pos, (++Shape::ControlPointHandle(start))->pos);

    Scalar tin           = 0;
    Scalar tout          = 0;
    int    sideIn        = -1;
    int    sideOut       = -1;
    int    mapSide[4][4] = {{2,3,0,1}, {1,2,3,0}, {0,1,2,3}, {3,0,1,2}};
    while (true)
    {
        patch->intersect(callback, ray, tin, sideIn, tout, sideOut);
        if (tout >= 1.0)
            break;

        //move to the patch on the exit side
        tin = tout;

        const Polyhedron* const polyhedron = DATAMANAGER->getPolyhedron();
        Polyhedron::Connectivity neighbors[4];
        polyhedron->getConnectivity(patch->getRootNode().node->index.patch,
                                    neighbors);
        patch  = renderPatches[neighbors[sideOut][0]];
        sideIn = mapSide[neighbors[sideOut][1]][sideOut];
    }
}


const FrameStamp& Crusta::
getLastScaleStamp() const
{
    return lastScaleStamp;
}

void Crusta::
setTexturingMode(int mode)
{
    texturingMode = mode;
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


GLColorMap* Crusta::
getColorMap()
{
    return colorMap;
}

void Crusta::
touchColorMap()
{
    colorMapDirty = true;
}

void Crusta::
uploadColorMap(GLuint colorTex)
{
    glBindTexture(GL_TEXTURE_1D, colorTex);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, colorMap->getNumEntries(), 0,
                 GL_RGBA, GL_FLOAT, colorMap->getColors());
}

Point3 Crusta::
mapToScaledGlobe(const Point3& pos)
{
    Vector3 toPoint(pos[0], pos[1], pos[2]);
    Vector3 onSurface(toPoint);
    onSurface.normalize();
    onSurface *= SETTINGS->globeRadius;
    toPoint   -= onSurface;
    toPoint   *= verticalScale;
    toPoint   += onSurface;

    return Point3(toPoint[0], toPoint[1], toPoint[2]);
}

Point3 Crusta::
mapToUnscaledGlobe(const Point3& pos)
{
    Vector3 toPoint(pos[0], pos[1], pos[2]);
    Vector3 onSurface(toPoint);
    onSurface.normalize();
    onSurface *= SETTINGS->globeRadius;
    toPoint   -= onSurface;
    toPoint   /= verticalScale;
    toPoint   += onSurface;

    return Point3(toPoint[0], toPoint[1], toPoint[2]);
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
///\todo hack. allow for the cache processing to happen as a display post-proc
    //process the requests from the last frame
    DATAMANAGER->frame();

    CURRENT_FRAME = Vrui::getApplicationTime();

///\todo hack. start the actual new frame
statsMan.newFrame();

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
        SETTINGS->decorateVectorArt = true;
    }
}
#endif //CRUSTA_ENABLE_DEBUG

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
        Point3 physicalCenter = Vrui::getDisplayCenter();
        const Vrui::NavTransform& navXform =
            Vrui::getNavigationTransformation();
        Point3 navCenter = navXform.inverseTransform(physicalCenter);

        Vector3 toCenter(navCenter[0], navCenter[1], navCenter[2]);
        Scalar height       = toCenter.mag();
        Scalar altitude     = (height - SETTINGS->globeRadius) / verticalScale;
        Scalar newHeight    = altitude*newVerticalScale + SETTINGS->globeRadius;
        Vector3 newToCenter = toCenter * (newHeight / height);
        Vector3 translation = toCenter - newToCenter;
        Vrui::setNavigationTransformation(navXform*
            Vrui::NavTransform::translate(translation));

        /* changes to the navigation only get applied in the next frame. Delay
           the processing to the frame that will have the proper navigation */
        changedVerticalScale = newVerticalScale;
    }

    //let the map manager update all the mapping stuff
    mapMan->frame();
}

void Crusta::
display(GLContextData& contextData)
{
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

statsMan.extractTileStats(surface);

CRUSTA_DEBUG(50, CRUSTA_DEBUG_OUT <<
"Number of nodes to render: " << surface.visibles.size() << "\n";)

    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    glPushAttrib(GL_TEXTURE_BIT);

    //update the map data
///\todo integrate properly (VIS 2010)
    if (SETTINGS->decorateVectorArt)
    {
        mapMan->updateLineData(surface);
        CHECK_GLA
    }

//- draw the current terrain and map data
///\todo integrate properly (VIS 2010)
//bind the texture that contains the symbol images
if (SETTINGS->decorateVectorArt)
{
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, glData->symbolTex);
    CHECK_GLA
}

    //bind the colormap texture
    if (texturingMode == 1)
    {
        //upload a modified color map
        if (colorMapDirty)
        {
            uploadColorMap(glData->colorMap);
            colorMapDirty = false;
        }

        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_1D, glData->colorMap);
        CHECK_GLA
    }

    //have the QuadTerrain draw the surface approximation
    CHECK_GLA

    //draw the terrain
    glData->terrainShader.setLinesDecorated(SETTINGS->decorateVectorArt);
    glData->terrainShader.setTexturingMode(texturingMode);
    glData->terrainShader.update();
    glData->terrainShader.enable();
    if (texturingMode == 1)
    {
        glData->terrainShader.setMinColorMapElevation(
            colorMap->getScalarRangeMin());
        glData->terrainShader.setColorMapElevationInvRange(
            1.0/(colorMap->getScalarRangeMax()-colorMap->getScalarRangeMin()));
    }
    glData->terrainShader.setVerticalScale(getVerticalScale());
    glData->terrainShader.setTextureStep(TILE_TEXTURE_COORD_STEP);
    glData->terrainShader.setDemNodata(DATAMANAGER->getDemNodata());
    const TextureColor::Type& cnd = DATAMANAGER->getColorNodata();
    glData->terrainShader.setColorNodata(cnd[0], cnd[1], cnd[2]);
    glData->terrainShader.setDemDefault(SETTINGS->terrainDefaultHeight);
    glData->terrainShader.setColorDefault(SETTINGS->terrainDefaultColor);

///\todo this needs to be tweakable
    if (SETTINGS->decorateVectorArt)
    {
        float scaleFac = Vrui::getNavigationTransformation().getScaling();
        glData->terrainShader.setLineCoordScale(scaleFac);
        float lineWidth = 0.1f / scaleFac;
        glData->terrainShader.setLineWidth(lineWidth);
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
    SETTINGS->decorateVectorArt = flag;
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


END_CRUSTA
