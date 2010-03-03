#include <crusta/Crusta.h>

#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/GLColorMap.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>

#include <crusta/checkGl.h>
#include <crusta/DataManager.h>
#include <crusta/map/MapManager.h>
#include <crusta/QuadCache.h>
#include <crusta/QuadTerrain.h>
#include <crusta/Section.h>
#include <crusta/Sphere.h>
#include <crusta/SurfaceTool.h>
#include <crusta/Tool.h>
#include <crusta/Triacontahedron.h>


#if DEBUG_INTERSECT_CRAP
#define DEBUG_INTERSECT_PEEK 0
#include <crusta/CrustaVisualizer.h>
#endif //DEBUG_INTERSECT_CRAP


BEGIN_CRUSTA

#if DEBUG_INTERSECT_CRAP
///\todo debug remove
bool DEBUG_INTERSECT = false;
#endif //DEBUG_INTERSECT_CRAP

void Crusta::
init(const std::string& demFileBase, const std::string& colorFileBase)
{
    //initialize the surface transformation tool
    SurfaceTool::init();
    //initialize the abstract crusta tool (adds an entry to the VRUI menu)
    Vrui::ToolFactory* crustaTool = Tool::init(NULL);

    /* start the frame counting at 2 because initialization code uses unsigneds
    that are initialized with 0. Thus if crustaFrameNumber starts at 0, the
    init code wouldn't be able to retrieve any cache buffers since all the
    buffers of the current and previous frame are locked */
    currentFrame     = 2;
    lastScaleFrame   = 2;
    texturingMode    = 2;
    verticalScale    = 0.0;
    newVerticalScale = 1.0;
    bufSize[0] = bufSize[1] = Math::Constants<int>::max;

    Triacontahedron polyhedron(SPHEROID_RADIUS);

    cache    = new Cache(4096, 1024, this);
    dataMan  = new DataManager(&polyhedron, demFileBase, colorFileBase, this);
    mapMan   = new MapManager(crustaTool, this);

    globalElevationRange[0] =  Math::Constants<Scalar>::max;
    globalElevationRange[1] = -Math::Constants<Scalar>::max;

    uint numPatches = polyhedron.getNumPatches();
    renderPatches.resize(numPatches);
    for (uint i=0; i<numPatches; ++i)
    {
        renderPatches[i] = new QuadTerrain(i, polyhedron.getScope(i), this);
        const QuadNodeMainData& root = renderPatches[i]->getRootNode();
        globalElevationRange[0] = std::min(globalElevationRange[0],
                                           Scalar(root.elevationRange[0]));
        globalElevationRange[1] = std::max(globalElevationRange[1],
                                           Scalar(root.elevationRange[1]));
    }

    colorMapDirty = true;
    static const int numColorMapEntries = 1024;
    GLColorMap::Color dummyColorMap[numColorMapEntries];
    colorMap = new GLColorMap(numColorMapEntries, dummyColorMap,
                              GLdouble(globalElevationRange[0]),
                              GLdouble(globalElevationRange[1]));
}

void Crusta::
shutdown()
{
    for (RenderPatches::iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        delete *it;
    }
    delete mapMan;
    delete dataMan;
    delete cache;
}

double Crusta::
getHeight(double x, double y, double z)
{
    Scope::Vertex p(x,y,z);

//- find the base patch
    MainCacheBuffer*  nodeBuf = NULL;
    QuadNodeMainData* node    = NULL;
    for (int patch=0; patch<static_cast<int>(renderPatches.size()); ++patch)
    {
        //grab specification of the patch
        nodeBuf = cache->getMainCache().findCached(TreeIndex(patch));
        assert(nodeBuf != NULL);
        node = &nodeBuf->getData();

        //check containment
        if (node->scope.contains(p))
            break;
    }

    assert(node->index.patch < static_cast<uint>(renderPatches.size()));

//- grab the finest level data possible
    while (true)
    {
        //is it even possible to retrieve higher res data?
        bool higherResDataFetchable = false;
        for (int i=0; i<4; ++i)
        {
            if (node->childDemTiles[i]  !=  DemFile::INVALID_TILEINDEX ||
                node->childColorTiles[i]!=ColorFile::INVALID_TILEINDEX)
            {
                higherResDataFetchable = true;
                break;
            }
        }
        if (!higherResDataFetchable)
            break;

        //try to grab the children for evaluation
        CacheRequests missingChildren;
        for (int i=0; i<4; ++i)
        {
            //find child
            TreeIndex childIndex      = node->index.down(i);
            MainCacheBuffer* childBuf =
                cache->getMainCache().findCached(childIndex);
            if (childBuf == NULL)
                missingChildren.push_back(CacheRequest(0, nodeBuf, i));
            else if (childBuf->getData().scope.contains(p))
            {
                //switch to the child for traversal continuation
                nodeBuf = childBuf;
                node    = &childBuf->getData();
                missingChildren.clear();
                break;
            }
        }
        if (!missingChildren.empty())
        {
            //request the missing and stop traversal
            cache->getMainCache().request(missingChildren);
            break;
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

    Scope scope = node->scope;
    int offset[2] = {0,0};
    int shift = (TILE_RESOLUTION-1)>>1;
    for (int level=1; level<numLevels; ++level)
    {
        //compute the coverage for the children
        Scope childScopes[4];
        scope.split(childScopes);

        //find the child containing the point
        for (int i=0; i<4; ++i)
        {
            if (childScopes[i].contains(p))
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
///\todo sample properly. For now just return the height of the corner
    double height = node->height[offset[1]*TILE_RESOLUTION + offset[0]];
    return height;
}

Point3 Crusta::
snapToSurface(const Point3& pos, Scalar elevationOffset)
{
//- find the base patch
    MainCacheBuffer*  nodeBuf = NULL;
    QuadNodeMainData* node    = NULL;
    for (int patch=0; patch<static_cast<int>(renderPatches.size()); ++patch)
    {
        //grab specification of the patch
        nodeBuf = cache->getMainCache().findCached(TreeIndex(patch));
        assert(nodeBuf != NULL);
        node = &nodeBuf->getData();

        //check containment
        if (node->scope.contains(pos))
            break;
        else
            node = NULL;
    }

    assert(node != NULL);
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
        if (node->childDemTiles[childId]   ==   DemFile::INVALID_TILEINDEX &&
            node->childColorTiles[childId] == ColorFile::INVALID_TILEINDEX)
        {
            break;
        }

        //try to grab the child for evaluation
        MainCache& mainCache      = cache->getMainCache();
        TreeIndex childIndex      = node->index.down(childId);
        MainCacheBuffer* childBuf = mainCache.findCached(childIndex);
        if (childBuf == NULL)
        {
            mainCache.request(CacheRequest(0.0, nodeBuf, childId));
            break;
        }
        else
        {
            //switch to the child for traversal continuation
            nodeBuf = childBuf;
            node    = &childBuf->getData();
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
///\todo sample properly. For now just return the height of the corner
    Scalar height = node->height[offset[1]*TILE_RESOLUTION + offset[0]];
    height       += SPHEROID_RADIUS + elevationOffset;

    Vector3 toPos = Vector3(pos);
    toPos.normalize();
    toPos *= height;
    return Point3(toPos[0], toPos[1], toPos[2]);
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

    const Scalar& verticalScale = getVerticalScale();

    //intersect the ray with the global outer shells to determine starting point
    Sphere shell(Point3(0), SPHEROID_RADIUS +
                 verticalScale*globalElevationRange[1]);
    Scalar gin, gout;
    bool intersects = shell.intersectRay(ray, gin, gout);

    if (!intersects)
        return HitResult();

    shell.setRadius(SPHEROID_RADIUS + verticalScale*globalElevationRange[0]);
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
    const QuadTerrain*      patch = NULL;
    const QuadNodeMainData* node  = NULL;
    for (RenderPatches::const_iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        node = &((*it)->getRootNode());
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
    Triacontahedron polyhedron(SPHEROID_RADIUS);
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

        Polyhedron::Connectivity neighbors[4];
        polyhedron.getConnectivity(patch->getRootNode().index.patch, neighbors);
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


const FrameNumber& Crusta::
getCurrentFrame() const
{
    return currentFrame;
}

const FrameNumber& Crusta::
getLastScaleFrame() const
{
    return lastScaleFrame;
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
    onSurface *= SPHEROID_RADIUS;
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
    onSurface *= SPHEROID_RADIUS;
    toPoint   -= onSurface;
    toPoint   /= verticalScale;
    toPoint   += onSurface;

    return Point3(toPoint[0], toPoint[1], toPoint[2]);
}

Cache* Crusta::
getCache() const
{
    return cache;
}

DataManager* Crusta::
getDataManager() const
{
    return dataMan;
}

MapManager* Crusta::
getMapManager() const
{
    return mapMan;
}

LightingShader& Crusta::
getTerrainShader(GLContextData& contextData)
{
    GlData* glData = contextData.retrieveDataItem<GlData>(this);
    return glData->terrainShader;
}


void Crusta::
submitActives(const Actives& touched)
{
    Threads::Mutex::Lock lock(activesMutex);
    actives.insert(actives.end(), touched.begin(), touched.end());
}


void Crusta::
frame()
{
    //check for scale changes since the last frame
    if (verticalScale  != newVerticalScale)
    {
        verticalScale  = newVerticalScale;
        lastScaleFrame = currentFrame;
    }

    ++currentFrame;

    DEBUG_OUT(8, "\n\n\n--------------------------------------\n%u\n\n\n",
              static_cast<unsigned int>(currentFrame));

    //make sure all the active nodes are current
    confirmActives();

    //process the requests from the last frame
    cache->getMainCache().frame();

    //let the map manager update all the mapping stuff
    mapMan->frame();
}

void Crusta::
display(GLContextData& contextData)
{
    CHECK_GLA

    GlData* glData = contextData.retrieveDataItem<GlData>(this);

    //upload a modified color map
    if (colorMapDirty)
    {
        uploadColorMap(glData->colorMap);
        colorMapDirty = false;
    }

    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_1D, glData->colorMap);

    glData->terrainShader.setTexturingMode(texturingMode);
    glData->terrainShader.update();
    glData->terrainShader.enable();
    glData->terrainShader.setMinColorMapElevation(
        colorMap->getScalarRangeMin());
    glData->terrainShader.setColorMapElevationInvRange(
        1.0 / (colorMap->getScalarRangeMax()-colorMap->getScalarRangeMin()));
    glData->terrainShader.setTextureStep(TILE_TEXTURE_COORD_STEP);
    glData->terrainShader.setVerticalScale(getVerticalScale());

    for (RenderPatches::const_iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        (*it)->display(contextData);
        CHECK_GLA
    }

    glData->terrainShader.disable();

    //let the map manager draw all the mapping stuff
    mapMan->display(contextData);
    CHECK_GLA

    glActiveTexture(activeTexture);
}


Crusta::GlData::
GlData()
{
    glGenTextures(1, &colorMap);
    glBindTexture(GL_TEXTURE_1D, colorMap);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Crusta::GlData::
~GlData()
{
    glDeleteTextures(1, &colorMap);
}

void Crusta::
confirmActives()
{
    for (Actives::iterator it=actives.begin(); it!=actives.end(); ++it)
    {
        if (!(*it)->isCurrent(this))
            (*it)->getData().computeBoundingSphere(getVerticalScale());
        (*it)->touch(this);
    }
    actives.clear();
}


void Crusta::
initContext(GLContextData& contextData) const
{
    GlData* glData = new GlData;
    contextData.addDataItem(this, glData);
}


END_CRUSTA
