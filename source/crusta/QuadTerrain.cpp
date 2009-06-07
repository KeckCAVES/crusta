#include <crusta/QuadTerrain.h>

#include <algorithm>
#include <assert.h>

#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <crusta/CacheRequest.h>
#include <crusta/Crusta.h>
#include <crusta/QuadCache.h>
///\todo remove
#include <crusta/simplexnoise1234.h>
#include <Misc/File.h>

BEGIN_CRUSTA

static const uint NUM_GEOMETRY_INDICES =
    (TILE_RESOLUTION-1)*(TILE_RESOLUTION*2 + 2) - 2;
static const float TEXTURE_COORD_STEP  = 1.0 / TILE_RESOLUTION;
static const float TEXTURE_COORD_START = TEXTURE_COORD_STEP * 0.5;
static const float TEXTURE_COORD_END   = 1.0 - TEXTURE_COORD_START;

bool QuadTerrain::displayDebuggingGrid = false;

QuadTerrain::
QuadTerrain(uint8 patch, const Scope& scope, const std::string& demFileName,
            const std::string& colorFileName) :
    demFile(NULL), colorFile(NULL), basePatchId(patch), baseScope(scope)
{
    uint res[2] = { TILE_RESOLUTION, TILE_RESOLUTION };
    if (!demFileName.empty())
        demFile   = new DemFile(demFileName.c_str(), res);
    if (!colorFileName.empty())
        colorFile = new ColorFile(colorFileName.c_str(), res);

    //create root data and pin it in the cache
    TreeIndex rootIndex(patch);
    MainCacheBuffer* rootBuffer =
        crustaQuadCache.getMainCache().getBuffer(rootIndex);
    assert(rootBuffer!=NULL);
    rootBuffer->pin();

    geometryBuf = new double[TILE_RESOLUTION*TILE_RESOLUTION*3];
}
QuadTerrain::
~QuadTerrain()
{
    delete[] geometryBuf;
    delete demFile;

    for (NodeBlocks::iterator it=nodePool.begin(); it!=nodePool.end(); ++it)
        delete[] *it;
    nodePool.clear();
}


void QuadTerrain::
display(GLContextData& contextData)
{
    //grab the context dependent active set
    GlData* glData = contextData.retrieveDataItem<GlData>(this);

    //setup the GL
    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    GLint arrayBuffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
    GLint elementArrayBuffer;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementArrayBuffer);

    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT);
	glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1.0f);

    glData->shader.useProgram();

    //make sure to update the tree for the new vertical scale
    if (glData->verticalScale != Crusta::getVerticalScale())
    {
        updateVerticalScale(glData->root);
        glData->verticalScale = Crusta::getVerticalScale();
        glUniform1f(glData->verticalScaleUniform, glData->verticalScale);
    }

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);

    //setup the evaluators
    glData->visibility.frustum.setFromGL();
    glData->lod.frustum = glData->visibility.frustum;
    glData->lod.setFocusFromDisplay();

    /* display could be multi-threaded. Buffer all the node data requests and
       merge them into the request list en block */
    CacheRequests dataRequests;

    /* save the current openGL transform. We are going to replace it during
       traversal with a scope centroid centered one to alleviate floating point
       issues with rotating vertices far off the origin */
    glPushMatrix();

    /* traverse the terrain tree, update as necessary and issue drawing commands
       for active nodes */
DEBUG_BEGIN(1)
checkTreeRoot(glData->root);
DEBUG_END
    traverse(contextData, glData, glData->root, dataRequests);
DEBUG_BEGIN(1)
checkTreeRoot(glData->root);
DEBUG_END
    draw(contextData, glData, glData->root);
DEBUG_BEGIN(1)
checkTreeRoot(glData->root);
DEBUG_END

    //restore the GL transform as it was before
    glPopMatrix();

    glData->shader.disablePrograms();

    glPopClientAttrib();
    glPopAttrib();

    glActiveTexture(activeTexture);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);

    //merge the data requests
    crustaQuadCache.getMainCache().request(dataRequests);
}

void QuadTerrain::
loadChildren(Node* node, Node* children, MainCacheBuffer* childBuffers[4],
             bool childCached[4])
{
    //compute the child scopes
    Scope childScopes[4];
    node->scope.split(childScopes);

    for (int i=0; i<4; ++i)
    {
        Node& child       = children[i];
        child.terrain     = node->terrain;
        child.index       = node->index.down(i);
        child.scope       = childScopes[i];
        child.demTile     = node->mainBuffer->getData().childDemTiles[i];
        child.colorTile   = node->mainBuffer->getData().childColorTiles[i];
        child.mainBuffer  = childBuffers[i];
        child.pinned      = false;
        child.parent      = node;
        child.children    = NULL;

        //need to source the data again only if it wasn't cached yet
        if (!childCached[i])
        {
            sourceDem(&child);
            sourceColor(&child);
            generateGeometry(&child);
        }
        else
            child.init();
    }
}

void QuadTerrain::
generateGeometry(Node* node)
{
    QuadNodeMainData::Vertex* vertices = node->mainBuffer->getData().geometry;

    //compute the average height and shell radius from the elevation range
#if USING_AVERAGE_HEIGHT
    DemHeight avgElevation = (node->elevationRange[0]+node->elevationRange[1]) *
                             DemHeight(0.5);
    double shellRadius = SPHEROID_RADIUS + avgElevation;
#else
    double shellRadius = SPHEROID_RADIUS;
#endif //USING_AVERAGE_HEIGHT
    node->scope.getRefinement(shellRadius, TILE_RESOLUTION, geometryBuf);

    /* compute and store the centroid here, since node-creation level generation
       of these values only happens after the data load step */
    Scope::Vertex scopeCentroid = node->scope.getCentroid(shellRadius);
    node->centroid[0] = scopeCentroid[0];
    node->centroid[1] = scopeCentroid[1];
    node->centroid[2] = scopeCentroid[2];

    QuadNodeMainData::Vertex* v = vertices;
    for (double* g=geometryBuf; g<geometryBuf+TILE_RESOLUTION*TILE_RESOLUTION*3;
         g+=3, ++v)
    {
        v->position[0] = DemHeight(g[0] - node->centroid[0]);
        v->position[1] = DemHeight(g[1] - node->centroid[1]);
        v->position[2] = DemHeight(g[2] - node->centroid[2]);
    }
}

template <typename PixelParam>
inline void
sampleParentBase(int child, PixelParam range[2], PixelParam* dst,
                 PixelParam* src)
{
    static const int offsets[4] = {
        0, (TILE_RESOLUTION-1)>>1, ((TILE_RESOLUTION-1)>>1)*TILE_RESOLUTION,
        ((TILE_RESOLUTION-1)>>1)*TILE_RESOLUTION + ((TILE_RESOLUTION-1)>>1) };
    int halfSize[2] = { (TILE_RESOLUTION+1)>>1, (TILE_RESOLUTION+1)>>1 };

    for (int y=0; y<halfSize[1]; ++y)
    {
        PixelParam* wbase = dst + y*2*TILE_RESOLUTION;
        PixelParam* rbase = src + y*TILE_RESOLUTION + offsets[child];

        for (int x=0; x<halfSize[0]; ++x, wbase+=2, ++rbase)
        {
            range[0] = pixelMin(range[0], rbase[0]);
            range[1] = pixelMax(range[1], rbase[0]);

            wbase[0] = rbase[0];
            if (x<halfSize[0]-1)
                wbase[1] = pixelAvg(rbase[0], rbase[1]);
            if (y<halfSize[1]-1)
            {
                wbase[TILE_RESOLUTION] = pixelAvg(rbase[0],
                                                  rbase[TILE_RESOLUTION]);
            }
            if (x<halfSize[0]-1 && y<halfSize[1]-1)
            {
                wbase[TILE_RESOLUTION+1] = pixelAvg(rbase[0], rbase[1],
                    rbase[TILE_RESOLUTION], rbase[TILE_RESOLUTION+1]);
            }
        }
    }
}

inline void
sampleParent(int child, DemHeight range[2], DemHeight* dst, DemHeight* src)
{
    range[0] = Math::Constants<DemHeight>::max;
    range[1] = Math::Constants<DemHeight>::min;

    sampleParentBase(child, range, dst, src);
}

inline void
sampleParent(int child, TextureColor range[2], TextureColor* dst,
             TextureColor* src)
{
    range[0] = TextureColor(255,255,255);
    range[1] = TextureColor(0,0,0);

    sampleParentBase(child, range, dst, src);
}

void QuadTerrain::
sourceDem(Node* node)
{
    DemHeight* heights = node->mainBuffer->getData().height;
    DemHeight* range   = &node->mainBuffer->getData().elevationRange[0];

    if (node->demTile!=DemFile::INVALID_TILEINDEX)
    {
        DemTileHeader header;
        if (!node->terrain->demFile->readTile(node->demTile,
            node->mainBuffer->getData().childDemTiles, header, heights))
        {
            Misc::throwStdErr("QuadTerrain::QuadTerrain: Invalid DEM file: "
                              "could not read node %s's data",
                              node->index.med_str().c_str());
        }
        range[0] = header.range[0];
        range[1] = header.range[1];
    }
    else
    {
        if (node->parent != NULL)
        {
            sampleParent(node->index.child, range, heights,
                         node->parent->mainBuffer->getData().height);
        }
        else
        {
            range[0] = range[1] = demNodata;
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                heights[i] = demNodata;
        }
    }
    node->init();
}

void QuadTerrain::
sourceColor(Node* node)
{
    TextureColor* colors = node->mainBuffer->getData().color;

    if (node->colorTile!=ColorFile::INVALID_TILEINDEX)
    {
        if (!node->terrain->colorFile->readTile(node->colorTile,
            node->mainBuffer->getData().childColorTiles, colors))
        {
            Misc::throwStdErr("QuadTerrain::QuadTerrain: Invalid Color "
                              "file: could not read node %s's data",
                              node->index.med_str().c_str());
        }
    }
    else
    {
        TextureColor range[2];
        if (node->parent != NULL)
        {
            sampleParent(node->index.child, range, colors,
                         node->parent->mainBuffer->getData().color);
        }
        else
        {
#if 0
#if 1

    Misc::File f("/home/saru/dev/proj/Crusta/bin/65pattern.rgb", "rb");
    f.read<uint8>((uint8*)(colors), 65*65*3);

#if 0
    srand(333);
    TextureColor palette[16];
    for (int i=0; i<16; ++i)
    {
        palette[i] = TextureColor(255*float(rand())/RAND_MAX,
                                  255*float(rand())/RAND_MAX,
                                  255*float(rand())/RAND_MAX);
    }

    int numInner = (((TILE_RESOLUTION-1)>>2) + 1) - 2;
    int p = 0;
    TextureColor* c = colors;
    for (int y=0; y<4; ++y)
    {
        //place border row
        for (int x=0; x<TILE_RESOLUTION; ++x, ++c)
            *c = TextureColor((y%2)*255, (y%2)*255, (y%2)*255);
        //do inners
        for (int i=0; i<numInner; ++i)
        {
            *c = palette[y*4
        }
    }
#endif

#else
int w = 8;
for (uint i=0; i<TILE_RESOLUTION; ++i)
{
    for (uint j=0; j<TILE_RESOLUTION; ++j)
    {
//        uint8 c = ((((i&w)==0)^((j&w))==0));
        TextureColor& tc = colors[i*TILE_RESOLUTION + j];
        tc[0] = 255 * (float(j)/TILE_RESOLUTION);
        tc[1] = 255 * (float(i)/TILE_RESOLUTION);
        tc[2] = 0;
    }
}
#endif
#else
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                colors[i] = colorNodata;
#endif
        }
    }
}

Node* QuadTerrain::
grabNodeBlock()
{
    if (!nodePool.empty())
    {
        Node* block = nodePool.back();
        nodePool.pop_back();
        return block;
    }
    else
        return new Node[4];
}

void QuadTerrain::
releaseNodeBlock(Node* children)
{
///\todo control size of dangling buffers
    nodePool.push_back(children);
}



///\todo debug, remove
void QuadTerrain::
printTree(Node* node)
{
    //recurse to leaves
    if (node->children != NULL)
    {
        for (uint i=0; i<4; ++i)
            printTree(&node->children[i]);
    }
    else
        DEBUG_OUT(1, "%s ", node->index.str().c_str());
}

void QuadTerrain::
checkTreeRoot(Node* node)
{
    if (node->parent != NULL)
    {
        TreeIndex parentChild = node->parent->index.down(node->index.child);
        TreeIndex childParent = node->index.up();
        assert(parentChild==node->index  &&  childParent==node->parent->index);
    }

    MainCacheBuffer* main =
        crustaQuadCache.getMainCache().findCached(node->index);
//just because some debuggers don't halt on the assert!!
    if (main==NULL && node->mainBuffer!=NULL)
    {
        std::cout<< "FRAKALICIOUS!";
        assert(false && "Main buffer not available");
    }
    if (main != node->mainBuffer)
    {
        std::cout<< "FRAKOWNED";
        assert(false && "cached and in-node main buffers differ");
    }

    if (node->children != NULL)
    {
        for (int i=0; i<4; ++i)
        {
            TreeIndex parentChild = node->index.down(i);
            assert(parentChild == node->children[i].index);
        }
        for (uint i=0; i<4; ++i)
            checkTreeRoot(&node->children[i]);
    }
}

void QuadTerrain::
checkTree(Node* node)
{
    //go up the tree to the root
    while (node->parent != NULL)
        node = node->parent;
    //check the whole tree
    checkTreeRoot(node);
}


void QuadTerrain::
updateVerticalScale(Node* node)
{
    node->computeBoundingSphere();
    if (node->children != NULL)
    {
        for (int i=0; i<4; ++i)
            node->children[i].computeBoundingSphere();
    }
}


bool QuadTerrain::
split(GlData* glData, Node* node, float lod, CacheRequests& requests)
{
    if (node->children != NULL)
        return true;

/**\todo Fix it: Need to disconnect the representation from the data as the
different data might not be available at the same resolution. For now only split
if there is data to support it */
    for (uint i=0; i<4; ++i)
    {
        //if there is no finer detail data, then there's no point refining
        if ((node->mainBuffer->getData().childDemTiles[i] ==
             DemFile::INVALID_TILEINDEX) &&
            (node->mainBuffer->getData().childColorTiles[i] ==
             ColorFile::INVALID_TILEINDEX))
        {
            return false;
        }
    }

#if 0
//first get the cache buffers required, as this might fail (cache full)
bool childCached[4];
TreeIndex childIndices[4];
MainCacheBuffer* childBuffers[4];
for (int i=0; i<4; ++i)
{
    childIndices[i] = node->index.down(i);
    childBuffers[i] = crustaQuadCache.getMainCache().getBuffer(childIndices[i],
                                                               &childCached[i]);
    assert(childBuffers[i] != NULL);
    childBuffers[i]->touch();
}
if (node->children != NULL)
{
for (int i=0; i<4; ++i)
{
    assert(node->children[i].index == childIndices[i]);
    assert(node->children[i].mainBuffer == childBuffers[i]);
}
}


//allocate the new children
Node* children = grabNodeBlock();

//fetch the data
loadChildren(node, children, childBuffers, childCached);
DEBUG_BEGIN(1)
checkTree(glData->root);
if (node->children != NULL)
{
for (int i=0; i<4; ++i)
{
    assert(node->children[i].index == children[i].index);
    assert(node->children[i].mainBuffer == children[i].mainBuffer);
}
}
DEBUG_END
if (node->children == NULL)
    node->children = children;
else
{
#if 1
    releaseNodeBlock(node->children);
    node->children = children;
#else
    releaseNodeBlock(children);
#endif
}
return true;
#endif

    //put out a request for the children of this node to be loaded
    requests.push_back(CacheRequest(lod, node));
    return false;
}

bool QuadTerrain::
discardSubTree(Node* node)
{
    assert(node->mainBuffer != NULL);
    if (node->children==NULL)
    {
        return node->mainBuffer->getFrameNumber()<crustaFrameNumber &&
               !node->pinned;
    }

///\todo WTF is this global crustaFrameNumber? why is it not a static of Crusta!
    //check the age of the subtree to allow for a delayed deallocation
    bool allowedToDiscard = true;
    for (int i=0; i<4; ++i)
        allowedToDiscard &= discardSubTree(&node->children[i]);
    if (!allowedToDiscard)
        return false;

    //return the children to the pool
    node->terrain->releaseNodeBlock(node->children);
    node->children = NULL;
    return true;
}

void QuadTerrain::
traverse(GLContextData& contextData, GlData* glData, Node* node,
         CacheRequests& requests)
{
    //traversal is top down until we get to the appropriate node
    //all ancester data is kept in the cache

    //make this node current
    node->mainBuffer->touch();

    float visible = glData->visibility.evaluate(node);
    if (visible)
    {
        assert(node->mainBuffer != NULL);

        //evaluate node for splitting
        float lod = glData->lod.evaluate(node);
        if (lod>1.0)
        {
            if (split(glData, node, lod, requests))
            {
                //continue traversal in the children
                for (uint i=0; i<4; ++i)
                    traverse(contextData, glData, &node->children[i], requests);
                return;
            }
        }
    }

    //check if my subtree can be eliminated
    discardSubTree(node);
}

const QuadNodeVideoData& QuadTerrain::
prepareGlData(GlData* glData, Node* node)
{
#if 0
    MainCacheBuffer* mainBuf =
        crustaQuadCache.getMainCache().findCached(node->index);
    VideoCacheBuffer* videoBuf = glData->videoCache.getStreamBuffer();

    const QuadNodeMainData&  mainData  =  mainBuf->getData();
    const QuadNodeVideoData& videoData = videoBuf->getData();
    //transfer the geometry
    glBindTexture(GL_TEXTURE_2D, videoData.geometry);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    TILE_RESOLUTION, TILE_RESOLUTION, GL_RGB, GL_FLOAT,
                    mainData.geometry);

    //transfer the evelation
    glBindTexture(GL_TEXTURE_2D, videoData.height);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    TILE_RESOLUTION, TILE_RESOLUTION, GL_RED, GL_FLOAT,
                    mainData.height);

    //transfer the color
    glBindTexture(GL_TEXTURE_2D, videoData.color);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    TILE_RESOLUTION, TILE_RESOLUTION, GL_RGB,
                    GL_UNSIGNED_BYTE, mainData.color);

    //return the data
    return videoData;
#endif

#if 1
    bool existed;
    VideoCacheBuffer* videoBuf = glData->videoCache.getBuffer(node->index,
                                                              &existed);
    if (existed)
    {
        //if there was already a match in the cache, just use that data
        videoBuf->touch();
        return videoBuf->getData();
    }
    else
    {
        //in any case the data has to be transfered from main memory
        if (videoBuf)
            videoBuf->touch();
        else
            videoBuf = glData->videoCache.getStreamBuffer();

        const QuadNodeMainData&  mainData  = node->mainBuffer->getData();
        const QuadNodeVideoData& videoData = videoBuf->getData();
        //transfer the geometry
        glBindTexture(GL_TEXTURE_2D, videoData.geometry);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        TILE_RESOLUTION, TILE_RESOLUTION, GL_RGB, GL_FLOAT,
                        mainData.geometry);

        //transfer the evelation
        glBindTexture(GL_TEXTURE_2D, videoData.height);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        TILE_RESOLUTION, TILE_RESOLUTION, GL_RED, GL_FLOAT,
                        mainData.height);

        //transfer the color
        glBindTexture(GL_TEXTURE_2D, videoData.color);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        TILE_RESOLUTION, TILE_RESOLUTION, GL_RGB,
                        GL_UNSIGNED_BYTE, mainData.color);

        //return the data
        return videoData;
    }
#endif
}

void QuadTerrain::
drawNode(GLContextData& contextData, GlData* glData, Node* node)
{
///\todo accommodate for lazy data fetching
    const QuadNodeVideoData& data = prepareGlData(glData, node);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data.geometry);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, data.height);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, data.color);

    glBindBuffer(GL_ARRAY_BUFFER,         glData->vertexAttributeTemplate);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glData->indexTemplate);

    glVertexPointer(2, GL_FLOAT, 0, 0);
    glIndexPointer(GL_SHORT, 0, 0);

#if 1
    //load the centroid relative translated navigation transformation
    glPushMatrix();
    Vrui::Vector centroidTranslation(
        node->centroid[0], node->centroid[1], node->centroid[2]);
    Vrui::NavTransform nav =
    Vrui::getDisplayState(contextData).modelviewNavigational;
    nav *= Vrui::NavTransform::translate(centroidTranslation);
    glLoadMatrix(nav);

    glUniform3f(glData->centroidUniform,
                node->centroid[0], node->centroid[1], node->centroid[2]);

    glDrawRangeElements(GL_TRIANGLE_STRIP, 0,
                        (TILE_RESOLUTION*TILE_RESOLUTION) - 1,
                        NUM_GEOMETRY_INDICES, GL_UNSIGNED_SHORT, 0);
    glPopMatrix();
#endif

#if 0
glBindBuffer(GL_ARRAY_BUFFER, 0);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
glVertexPointer(3, GL_FLOAT, 0, node->mainBuffer->getData().geometry);
glDrawArrays(GL_POINTS, 0, TILE_RESOLUTION*TILE_RESOLUTION);
#endif

#if 0
glData->shader.disablePrograms();
    Scope::Vertex* c = node->scope.corners;
    glDisable(GL_LIGHTING);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(c[0][0], c[0][1], c[0][2]);
        glColor3f(1.0f, 1.0f, 0.0f);
        glVertex3f(c[1][0], c[1][1], c[1][2]);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(c[3][0], c[3][1], c[3][2]);
        glColor3f(1.0f, 1.0f, 0.0f);
        glVertex3f(c[2][0], c[2][1], c[2][2]);
//        glColor3f(0.0f, 0.0f, 1.0f);
//        glVertex3f(c[0][0], c[0][1], c[0][2]);
    glEnd();
    glEnable(GL_TEXTURE_2D);
glData->shader.useProgram();
#endif

if (displayDebuggingGrid)
{
glData->shader.disablePrograms();
    Scope::Vertex* c = node->scope.corners;
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINE_STRIP);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(c[0][0], c[0][1], c[0][2]);
        glColor3f(1.0f, 1.0f, 0.0f);
        glVertex3f(c[1][0], c[1][1], c[1][2]);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(c[3][0], c[3][1], c[3][2]);
        glColor3f(0.0f, 1.0f, 1.0f);
        glVertex3f(c[2][0], c[2][1], c[2][2]);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(c[0][0], c[0][1], c[0][2]);
    glEnd();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
glData->shader.useProgram();
}

}

void QuadTerrain::
draw(GLContextData& contextData, GlData* glData, Node* node)
{
#if 1
bool isActiveChild=false;
if (node->children != NULL)
{
    for (int i=0; i<4; ++i)
    {
        if (node->children[i].mainBuffer->getFrameNumber()>=crustaFrameNumber)
        {
            isActiveChild = true;
            break;
        }
    }
}
if (isActiveChild)
{
    for (int i=0; i<4; ++i)
        draw(contextData, glData, &node->children[i]);
    return;
}
else
{
    drawNode(contextData, glData, node);
    return;
}
#endif

    if (node->mainBuffer->getFrameNumber() < crustaFrameNumber)
        return;

    //recurse if any of the children are current
    if (node->children != NULL)
    {
        for (int i=0; i<4; ++i)
            draw(contextData, glData, &node->children[i]);
        return;
    }

    drawNode(contextData, glData, node);
}




QuadTerrain::GlData::
GlData(QuadTerrain* terrain, const TreeIndex& iRootIndex,
       MainCacheBuffer* iRootBuffer, const Scope& baseScope,
       VideoCache& iVideoCache) :
    root(NULL), videoCache(iVideoCache), verticalScale(1.0),
    vertexAttributeTemplate(0), indexTemplate(0), verticalScaleUniform(0),
    centroidUniform(0)
{
    //initialize the static gl buffer templates
    generateVertexAttributeTemplate();
    generateIndexTemplate();

    //initialize the active front to be just the root node
    root = new Node;
    root->terrain    = terrain;
    root->index      = iRootIndex;
    root->scope      = baseScope;
    root->mainBuffer = iRootBuffer;

    for (int i=0; i<4; ++i)
    {
        root->mainBuffer->getData().childDemTiles[i] =
            DemFile::INVALID_TILEINDEX;
        root->mainBuffer->getData().childColorTiles[i] =
            ColorFile::INVALID_TILEINDEX;
    }

    if (root->terrain->demFile != NULL)
    {
    	root->demTile = 0;
        root->terrain->demNodata =
            root->terrain->demFile->getDefaultPixelValue();
    }
    else
        root->terrain->demNodata = DemHeight(-1);
    root->terrain->sourceDem(root);

    if (root->terrain->colorFile != NULL)
    {
    	root->colorTile = 0;
        root->terrain->colorNodata =
            root->terrain->colorFile->getDefaultPixelValue();
    }
    else
        root->terrain->colorNodata = TextureColor(255,255,255);
    root->terrain->sourceColor(root);

/**\todo the geometry generation is now dependent on the data!! Need to load it
in before the geometry can be generated. So long as the data is not loaded yet
use the values of the node from which the data is being sampled */
    root->terrain->generateGeometry(root);

    //initialize the shader
//    shader.compileVertexShader("elevation.vs");
    shader.compileVertexShader((std::string(CRUSTA_SHARE_PATH) +
                                "/litElevation.vs").c_str());
    shader.compileFragmentShader((std::string(CRUSTA_SHARE_PATH) +
                                  "/elevation.fs").c_str());
    shader.linkShader();
    shader.useProgram();

    verticalScaleUniform =
        shader.getUniformLocation("verticalScale");
    glUniform1f(verticalScaleUniform, 1);
    centroidUniform = shader.getUniformLocation("centroid");

    GLint uniform;
    uniform = shader.getUniformLocation("geometryTex");
    glUniform1i(uniform, 0);
    uniform = shader.getUniformLocation("heightTex");
    glUniform1i(uniform, 1);
    uniform = shader.getUniformLocation("colorTex");
    glUniform1i(uniform, 2);
    uniform = shader.getUniformLocation("texStep");
    glUniform1f(uniform, TEXTURE_COORD_STEP);

    shader.disablePrograms();

///\todo debug, remove: makes LOD recommend very coarse
//    lod.bias = -1.0;
}

QuadTerrain::GlData::
~GlData()
{
    QuadTerrain::discardSubTree(root);
    delete root;

    if (vertexAttributeTemplate != 0)
        glDeleteBuffers(1, &vertexAttributeTemplate);
    if (indexTemplate != 0)
        glDeleteBuffers(1, &indexTemplate);
}

void QuadTerrain::GlData::
generateVertexAttributeTemplate()
{
    /** allocate some temporary main memory to store the vertex attributes
        before they are streamed to the GPU */
    uint numTexCoords = TILE_RESOLUTION*TILE_RESOLUTION*2;
    float* positionsInMemory = new float[numTexCoords];
    float* positions = positionsInMemory;

    /** generate a set of normalized texture coordinates */
    for (float y = TEXTURE_COORD_START;
         y < (TEXTURE_COORD_END + 0.1*TEXTURE_COORD_STEP);
         y += TEXTURE_COORD_STEP)
    {
        for (float x = TEXTURE_COORD_START;
             x < (TEXTURE_COORD_END + 0.1*TEXTURE_COORD_STEP);
             x += TEXTURE_COORD_STEP, positions+=2)
        {
            positions[0] = x;
            positions[1] = y;
        }
    }

    //generate the vertex buffer and stream in the data
    glGenBuffers(1, &vertexAttributeTemplate);
    glBindBuffer(GL_ARRAY_BUFFER, vertexAttributeTemplate);
    glBufferData(GL_ARRAY_BUFFER, numTexCoords*sizeof(float), positionsInMemory,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //clean-up
    delete[] positionsInMemory;
}

void QuadTerrain::GlData::
generateIndexTemplate()
{
    /* allocate some temporary main memory to store the indices before they are
       streamed to the GPU */
    uint16* indicesInMemory = new uint16[NUM_GEOMETRY_INDICES];

    /* generate a sequence of indices that describe a single triangle strip that
       zizag through the geometry a row at a time: e.g.
              12 ...
              |
       10 11 13 - 14
              9 -  7
              | /  |
              8  -  4 5 6
              0  - 3
              |  / |
              1  - 2             */
    int  inc        = 1;
    uint alt        = 1;
    uint index[2]   = {0, TILE_RESOLUTION};
    uint16* indices = indicesInMemory;
    for (uint b=0; b<TILE_RESOLUTION-1; ++b, inc=-inc, alt=1-alt,
         index[0]+=TILE_RESOLUTION, index[1]+=TILE_RESOLUTION)
    {
        for (uint i=0; i<TILE_RESOLUTION*2;
             ++i, index[alt]+=inc, alt=1-alt, ++indices)
        {
            *indices = index[alt];
        }
        index[0]-=inc; index[1]-=inc;
        if (b != TILE_RESOLUTION-2)
        {
            for (uint i=0; i<2; ++i, ++indices)
                *indices = index[1];
        }
    }

    //generate the index buffer and stream in the data
    glGenBuffers(1, &indexTemplate);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexTemplate);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, NUM_GEOMETRY_INDICES*sizeof(uint16),
                 indicesInMemory, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //clean-up
    delete[] indicesInMemory;
}



void QuadTerrain::
initContext(GLContextData& contextData) const
{
    /* retrieve the root node's shared data from the cache. Must have been
     fetched/generated during construction and locked in the cache */
    TreeIndex rootIndex(basePatchId);
    MainCacheBuffer* rootBuffer =
        crustaQuadCache.getMainCache().findCached(rootIndex);
    assert(rootBuffer != NULL);

    //allocate the context dependent data
    GlData* glData = new GlData(const_cast<QuadTerrain*>(this), rootIndex,
                                rootBuffer, baseScope,
                                crustaQuadCache.getVideoCache(contextData));
    //commit the context data
    contextData.addDataItem(this, glData);
}


END_CRUSTA
