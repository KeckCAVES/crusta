#include <crusta/Crusta.h>

#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>

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
    currentFrame    = 2;
    lastScaleFrame  = 2;
    verticalScale   = 1.0;
    bufSize[0] = bufSize[1] = Math::Constants<int>::max;

    Triacontahedron polyhedron(SPHEROID_RADIUS);

    cache    = new Cache(8192, 4096, this);
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
setVerticalScale(double newVerticalScale)
{
    verticalScale  = newVerticalScale;
    lastScaleFrame = currentFrame;
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
    GlData* glData = contextData.retrieveDataItem<GlData>(this);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, glData->frameBuf);

    for (RenderPatches::const_iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        (*it)->display(contextData);
    }

    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    glPushAttrib(GL_TEXTURE_BIT);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glData->colorBuf);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, glData->depthBuf);

    glData->compositeShader.apply();

    //let the map manager draw all the mapping stuff
    mapMan->display(contextData);

    glPopAttrib();
    glActiveTexture(activeTexture);
}


Crusta::GlData::
GlData()
{
    glGenTextures(1, &colorBuf);
    glBindTexture(GL_TEXTURE_2D, colorBuf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    
    glGenTextures(1, &depthBuf);
    glBindTexture(GL_TEXTURE_2D, depthBuf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glGenFramebuffersEXT(1, &frameBuf);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameBuf);
    
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_2D, colorBuf, 0);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                              GL_TEXTURE_2D, depthBuf, 0);
    
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

Crusta::GlData::
~GlData()
{
    glDeleteTextures(1, &colorBuf);
    glDeleteTextures(1, &depthBuf);
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
prepareBuffers(GlData* glData)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    if (viewport[2]!=bufSize[0] || viewport[3]!=bufSize[1])
    {
        glBindTexture(GL_TEXTURE_2D, glData->colorBuf);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, viewport[2],
                     viewport[3], 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glBindTexture(GL_TEXTURE_2D, glData->depthBuf);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, viewport[2],
                     viewport[3], 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

        bufSize[0] = viewport[2];
        bufSize[1] = viewport[3];
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, glData->frameBuf);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
