#include <crusta/Crusta.h>

#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Misc/ThrowStdErr.h>
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

CrustaGlData::
CrustaGlData()
{
    QuadTerrain::generateVertexAttributeTemplate(vertexAttributeTemplate);
    QuadTerrain::generateIndexTemplate(indexTemplate);
    
    /* Check for the required OpenGL extensions: */
    if(!GLEXTFramebufferObject::isSupported())
        Misc::throwStdErr("Crusta: GL_EXT_framebuffer_object not supported");
    /* Initialize the required extensions: */
    GLEXTFramebufferObject::initExtension();
    
    glGenRenderbuffersEXT(1, &colorBuf);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, colorBuf);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA, 1,1);
    CHECK_GLA
    
    glGenTextures(1, &terrainAttributesTex);
    glBindTexture(GL_TEXTURE_2D, terrainAttributesTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    CHECK_GLA
    
    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, 1, 1, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    CHECK_GLA
    
    //create the framebuffer to be used for rendering the terrain
    glGenFramebuffersEXT(1, &colorTerrainDepthFrame);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, colorTerrainDepthFrame);
    CHECK_GLA
    
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                 GL_RENDERBUFFER_EXT, colorBuf);
    CHECK_GLA
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
                              GL_TEXTURE_2D, terrainAttributesTex, 0);
    CHECK_GLA
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                              GL_TEXTURE_2D, depthTex, 0);
    CHECK_GLA
    
    assert(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) ==
           GL_FRAMEBUFFER_COMPLETE_EXT);
    
    //create the framebuffer to be used for rendering the maps
    glGenFramebuffersEXT(1, &colorFrame);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, colorFrame);
    CHECK_GLA
    
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                 GL_RENDERBUFFER_EXT, colorBuf);
    CHECK_GLA
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    CHECK_GLA
    assert(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) ==
           GL_FRAMEBUFFER_COMPLETE_EXT);
    
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

CrustaGlData::
~CrustaGlData()
{
    glDeleteBuffers(1, &vertexAttributeTemplate);
    glDeleteBuffers(1, &indexTemplate);
    
    glDeleteRenderbuffersEXT(1, &colorBuf);
    glDeleteTextures(1, &terrainAttributesTex);
    glDeleteTextures(1, &depthTex);
    glDeleteFramebuffersEXT(1, &colorFrame);
    glDeleteFramebuffersEXT(1, &colorTerrainDepthFrame);
}



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
    currentFrame      = 2;
    lastScaleFrame    = 2;
    isTexturedTerrain = true;
    verticalScale     = 0.0;
    newVerticalScale  = 1.0;
    bufferSize[0] = bufferSize[1] = Math::Constants<int>::max;

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
/**\todo For the ray-casting we actually need min/max for the root.
    HUGE HACK, set ranges here to some defaults */
globalElevationRange[0] = -8000.0;
globalElevationRange[1] = 11000.0;
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
        MainCache::Requests missingChildren;
        for (int i=0; i<4; ++i)
        {
            //find child
            TreeIndex childIndex      = node->index.down(i);
            MainCacheBuffer* childBuf =
                cache->getMainCache().findCached(childIndex);
            if (childBuf == NULL)
                missingChildren.push_back(MainCache::Request(0, nodeBuf, i));
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
            mainCache.request(MainCache::Request(0.0, nodeBuf, childId));
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
useTexturedTerrain(bool useTex)
{
    isTexturedTerrain = useTex;
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
    CrustaGlData* glData = contextData.retrieveDataItem<CrustaGlData>(this);
    glData->videoCache = &getCache()->getVideoCache(contextData);

//- prepare the renderable representation
    std::vector<QuadNodeMainData*> renderNodes;

    //generate the terrain representation
    for (RenderPatches::const_iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        (*it)->prepareDisplay(contextData, renderNodes);
        CHECK_GLA
    }

///\todo generate the map representation

//- draw the current terrain and map data
    //have the QuadTerrain draw the surface approximation
    CHECK_GLA
    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);
    glPushAttrib(GL_TEXTURE_BIT);
    
    prepareFrameBuffer(glData);
    CHECK_GLA

    //draw the terrain
    glData->terrainShader.useTextureForColor(isTexturedTerrain);
    glData->terrainShader.update();
    glData->terrainShader.enable();
    glData->terrainShader.setTextureStep(TILE_TEXTURE_COORD_STEP);
    glData->terrainShader.setVerticalScale(getVerticalScale());

    QuadTerrain::display(contextData, glData, renderNodes);
    
    glData->terrainShader.disable();

    //let the map manager draw all the mapping stuff
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glData->depthTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, glData->terrainAttributesTex);
    
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, glData->colorFrame);

    mapMan->display(contextData);
    CHECK_GLA

    //commit the frame buffer to the on-screen one
    commitFrameBuffer(glData);
    CHECK_GLA
    
    glPopAttrib();
    glActiveTexture(activeTexture);
}


void Crusta::
confirmActives()
{
    MainCache& mainCache = getCache()->getMainCache();
    for (Actives::iterator it=actives.begin(); it!=actives.end(); ++it)
    {
        if (!mainCache.isCurrent(*it))
            (*it)->getData().computeBoundingSphere(getVerticalScale());
        mainCache.touch(*it);
    }
    actives.clear();
}


void Crusta::
prepareFrameBuffer(CrustaGlData* glData)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    if (viewport[2]!=bufferSize[0] || viewport[3]!=bufferSize[1])
    {
        bufferSize[0] = viewport[2];
        bufferSize[1] = viewport[3];

        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, glData->colorBuf);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA,
                                 bufferSize[0], bufferSize[1]);
        CHECK_GLA
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

        glBindTexture(GL_TEXTURE_2D, glData->terrainAttributesTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferSize[0], bufferSize[1], 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        CHECK_GLA

        glBindTexture(GL_TEXTURE_2D, glData->depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
                     bufferSize[0], bufferSize[1], 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        CHECK_GLA

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,
                             glData->colorTerrainDepthFrame);
        assert(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) ==
               GL_FRAMEBUFFER_COMPLETE_EXT);
    }

    //copy the content of the default frame buffer
    glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT,
                         glData->colorTerrainDepthFrame);
    CHECK_GLA

    glBlitFramebufferEXT(0, 0, bufferSize[0], bufferSize[1],
                         0, 0, bufferSize[0], bufferSize[1],
                         GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    CHECK_GLA

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, glData->colorTerrainDepthFrame);

    //clear the terrain attributes
    glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    CHECK_GLA

    static const GLenum bufs[2] = { GL_COLOR_ATTACHMENT0_EXT,
                                    GL_COLOR_ATTACHMENT1_EXT };
    glDrawBuffers(2, bufs);
    CHECK_GLA
}

void Crusta::
commitFrameBuffer(CrustaGlData *glData)
{
    glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT,
                         glData->colorTerrainDepthFrame);
    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
    CHECK_GLA

    glBlitFramebufferEXT(0, 0, bufferSize[0], bufferSize[1],
                         0, 0, bufferSize[0], bufferSize[1],
                         GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    CHECK_GLA

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void Crusta::
initContext(GLContextData& contextData) const
{
    CrustaGlData* glData   = new CrustaGlData();
    contextData.addDataItem(this, glData);
}


END_CRUSTA
