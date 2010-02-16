#include <crusta/Crusta.h>

#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>

#include <crusta/checkGl.h>
#include <crusta/DataManager.h>
#include <crusta/map/MapManager.h>
#include <crusta/QuadCache.h>
#include <crusta/QuadTerrain.h>
#include <crusta/SurfaceTool.h>
#include <crusta/Tool.h>
#include <crusta/Triacontahedron.h>

BEGIN_CRUSTA

void Crusta::
init(const std::string& demFileBase, const std::string& colorFileBase)
{
    //initialize the abstract crusta tool (adds an entry to the VRUI menu)
    Vrui::ToolFactory* crustaTool = Tool::init(NULL);
    //initialize the surface transformation tool
    SurfaceTool::init();

    /* start the frame counting at 2 because initialization code uses unsigneds
    that are initialized with 0. Thus if crustaFrameNumber starts at 0, the
    init code wouldn't be able to retrieve any cache buffers since all the
    buffers of the current and previous frame are locked */
    currentFrame     = 2;
    lastScaleFrame   = 2;
    verticalScale    = 0.0;
    newVerticalScale = 1.0;
    bufferSize[0] = bufferSize[1] = Math::Constants<int>::max;

    Triacontahedron polyhedron(SPHEROID_RADIUS);

    cache    = new Cache(4096, 1024, this);
    dataMan  = new DataManager(&polyhedron, demFileBase, colorFileBase, this);
    mapMan   = new MapManager(crustaTool);

    uint numPatches = polyhedron.getNumPatches();
    renderPatches.resize(numPatches);
    for (uint i=0; i<numPatches; ++i)
        renderPatches[i] = new QuadTerrain(i, polyhedron.getScope(i), this);
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
    height       *= Crusta::getVerticalScale();
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
    height       += elevationOffset;
    height       *= Crusta::getVerticalScale();

    Vector3 toCorner = Vector3(scope.corners[0]);
    toCorner.normalize();
    return scope.corners[0] + height*toCorner;
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
setVerticalScale(double nVerticalScale)
{
    newVerticalScale = nVerticalScale;
}

double Crusta::
getVerticalScale() const
{
    return verticalScale;
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
    CHECK_GLA

    GlData* glData = contextData.retrieveDataItem<GlData>(this);
    prepareFrameBuffer(glData);
    CHECK_GLA

    glPushAttrib(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);

    //draw the terrain
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    for (RenderPatches::const_iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        (*it)->display(contextData);
        CHECK_GLA
    }

    //let the map manager draw all the mapping stuff
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    mapMan->display(contextData);
    CHECK_GLA

    glPopAttrib();

    //commit the frame buffer to the on-screen one
    commitFrameBuffer(glData);
    CHECK_GLA
}


Crusta::GlData::
GlData()
{
    glGenRenderbuffersEXT(2, attachments);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, attachments[0]);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA, 1,1);
    CHECK_GLA
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, attachments[1]);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, 1,1);
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

    glGenFramebuffersEXT(1, &frameBuf);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameBuf);
    CHECK_GLA

    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                 GL_RENDERBUFFER_EXT, attachments[0]);
    CHECK_GLA
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
                              GL_TEXTURE_2D, terrainAttributesTex, 0);
    CHECK_GLA
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, attachments[1]);
    CHECK_GLA
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, attachments[1]);
    CHECK_GLA

    assert(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) ==
           GL_FRAMEBUFFER_COMPLETE_EXT);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

Crusta::GlData::
~GlData()
{
    glDeleteRenderbuffersEXT(3, attachments);
    glDeleteTextures(1, &terrainAttributesTex);
    glDeleteFramebuffersEXT(1, &frameBuf);
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
prepareFrameBuffer(GlData* glData)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    if (viewport[2]!=bufferSize[0] || viewport[3]!=bufferSize[1])
    {
        bufferSize[0] = viewport[2];
        bufferSize[1] = viewport[3];

        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, glData->attachments[0]);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA,
                                 bufferSize[0], bufferSize[1]);
        CHECK_GLA
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, glData->attachments[1]);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT,
                                 bufferSize[0], bufferSize[1]);
        CHECK_GLA
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

        glBindTexture(GL_TEXTURE_2D, glData->terrainAttributesTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferSize[0], bufferSize[1], 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, 0);
        CHECK_GLA

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, glData->frameBuf);
        assert(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) ==
               GL_FRAMEBUFFER_COMPLETE_EXT);
    }

    //copy the content of the default frame buffer
    glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, glData->frameBuf);
    CHECK_GLA

    glBlitFramebufferEXT(0, 0, bufferSize[0], bufferSize[1],
                         0, 0, bufferSize[0], bufferSize[1],
                         GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    CHECK_GLA

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, glData->frameBuf);

    //clear the stencil and terrain attributes
    glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
    glClearColor(1,0, 1.0, 1.0);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    CHECK_GLA

    static const GLenum bufs[2] = { GL_COLOR_ATTACHMENT0_EXT,
                                    GL_COLOR_ATTACHMENT1_EXT };
    glDrawBuffers(2, bufs);
    CHECK_GLA
}

void Crusta::
commitFrameBuffer(GlData *glData)
{
    glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, glData->frameBuf);
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
    if (!GLEXTFramebufferObject::isSupported())
    {
        Misc::throwStdErr("Crusta::initContext: required framebuffer object"
                          "extension is not supported");
    }

    GLEXTFramebufferObject::initExtension();

    GlData* glData = new GlData;
    contextData.addDataItem(this, glData);
}


END_CRUSTA
