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
    delete geometryBuf;
    delete demFile;
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
checkTree(glData, glData->root);
DEBUG_END
    traverse(contextData, glData, glData->root, dataRequests);
DEBUG_BEGIN(1)
checkTree(glData, glData->root);
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
generateGeometry(Node* node, QuadNodeMainData::Vertex* vertices)
{
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

void QuadTerrain::
sourceDem(Node* node, DemHeight* elevations)
{
    if (node->terrain->demFile != NULL)
    {
        if (node->demTile!=DemFile::INVALID_TILEINDEX)
        {
            DemTileHeader header;
            if (!node->terrain->demFile->readTile(
                node->demTile, node->childDemTiles, header, elevations))
            {
                Misc::throwStdErr("QuadTerrain::QuadTerrain: Invalid DEM file: "
                                  "could not read node %s's data",
                                  node->index.med_str().c_str());
            }
            node->init(header.range);
        }
    }
    else
    {
        if (elevations != NULL)
        {
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                elevations[i] = demNodata;
        }
        DemHeight range[2] = {demNodata, demNodata};
        node->init(range);
    }
}

void QuadTerrain::
sourceColor(Node* node, TextureColor* colors)
{
    if (node->terrain->colorFile != NULL)
    {
        if (node->colorTile!=ColorFile::INVALID_TILEINDEX)
        {
            if (!node->terrain->colorFile->readTile(
                node->colorTile, node->childColorTiles, colors))
            {
                Misc::throwStdErr("QuadTerrain::QuadTerrain: Invalid Color "
                                  "file: could not read node %s's data",
                                  node->index.med_str().c_str());
            }
        }
    }
    else
    {
        if (colors != NULL)
        {
            for (uint i=0; i<TILE_RESOLUTION*TILE_RESOLUTION; ++i)
                colors[i] = colorNodata;
        }
    }
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
checkTree(GlData* glData, Node* node)
{
    if (node->children != NULL)
    {
        for (uint i=0; i<4; ++i)
            checkTree(glData, &node->children[i]);
    }
    else
    {
        MainCacheBuffer* main =
            crustaQuadCache.getMainCache().findCached(node->index);
        if (main == NULL)
            std::cout<< "FRAKALICIOUS!";
        if (main != node->mainBuffer)
            std::cout<< "FRAKOWNED";
        if (node->videoBuffer != NULL)
        {
            VideoCacheBuffer* video =
                glData->videoCache.findCached(node->index);
            if (node->videoBuffer != video)
                std::cout<< "FRAKORAMA";
        }
    }
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
merge(GlData* glData, Node* node, float lod,
      CacheRequests& requests)
{
    //if we don't have the parent's data then we must request it
    node->parent->mainBuffer =
        crustaQuadCache.getMainCache().findCached(node->parent->index);
    if (node->parent->mainBuffer == NULL)
    {
        requests.push_back(CacheRequest(lod, node->parent));
        return false;
    }

    //make the parent current
    node->parent->videoBuffer = NULL;
DEBUG_OUT(1, "-Merge: %s:\n", node->parent->index.str().c_str());
    discardSubTree(glData, node->parent);
DEBUG_BEGIN(1)
printTree(glData->root); DEBUG_OUT(1, "\n\n");
checkTree(glData, glData->root);
DEBUG_END
    return true;
}

void QuadTerrain::
computeChildScopes(Node* node)
{
    assert(node->children!=NULL);
    Scope childScopes[4];
    node->scope.split(childScopes);

    for (uint i=0; i<4; ++i)
        node->children[i].scope = childScopes[i];
}

bool QuadTerrain::
split(GlData* glData, Node* node, float lod, CacheRequests& requests)
{
/**\todo Fix it: Need to disconnect the representation from the data as the
different data might not be available at the same resolution. For now only split
if there is data to support it */
    for (uint i=0; i<4; ++i)
    {
        //if there is no finer detail data, then there's no point refining
        if (node->childDemTiles[i]   ==   DemFile::INVALID_TILEINDEX &&
            node->childColorTiles[i] == ColorFile::INVALID_TILEINDEX)
        {
            return false;
        }
    }

    //tentatively assemble the children
    node->children = glData->grabNodeBlock();
    computeChildScopes(node);

    //verify that the critical data of the children is cached
    bool allChildrenCached = true;
    for (uint i=0; i<4; ++i)
    {
        Node& child = node->children[i];
        child.terrain    = node->terrain;
        child.parent     = node;
        child.index      = node->index.down(i);
        child.demTile    = node->childDemTiles[i];
        child.colorTile  = node->childColorTiles[i];
        child.mainBuffer =
            crustaQuadCache.getMainCache().findCached(child.index);
        if (child.mainBuffer == NULL)
        {
            requests.push_back(CacheRequest(lod, &child));
            allChildrenCached = false;
        }
        child.videoBuffer = NULL;
    }

    if (!allChildrenCached)
    {
        //the data for the children is missing, split-op must be aborted
        glData->releaseNodeBlock(node->children);
        node->children = NULL;
        return false;
    }
    else
    {
    //- split was successful
        /* source the meta-data for the elevation and color */
        for (uint i=0; i<4; ++i)
        {
            Node* child = &node->children[i];
            sourceDem(child);
            sourceColor(child);
        }
/** \todo wait... since the buffer can dissappear under the node between any
frame with no prior notice one can never assume these things are there and must
always ask the cache!! so this is pointless here.

At the same time why ask the cache all the time while nothing changes. Need a
better solution here in general */
        /* invalidate the buffers of the current node, since they might be
           reused for another node */
        node->mainBuffer  = NULL;
        node->videoBuffer = NULL;
DEBUG_OUT(1, "+Split: %s:\n", node->index.str().c_str());
DEBUG_BEGIN(1)
printTree(glData->root); DEBUG_OUT(1, "\n\n");
checkTree(glData, glData->root);
DEBUG_END
        return true;
    }
}


void QuadTerrain::
confirmCurrent(Node* node, float lod, CacheRequests& requests)
{
    //touch the current node
    node->mainBuffer->touch();
    if (node->parent != NULL)
    {
        //touch/request parent
        Node* p = node->parent;
        if (p->mainBuffer != NULL)
            p->mainBuffer->touch();
        else
        {
            p->mainBuffer = crustaQuadCache.getMainCache().findCached(p->index);
            if (p->mainBuffer == NULL)
            {
                requests.push_back(CacheRequest(lod, p));
            }
        }

        if (node->parent->parent != NULL)
        {
            //touch/request parent's parent
            p = node->parent->parent;
            if (p->mainBuffer != NULL)
                p->mainBuffer->touch();
            else
            {
                p->mainBuffer =
                    crustaQuadCache.getMainCache().findCached(p->index);
                if (p->mainBuffer == NULL)
                {
                    requests.push_back(CacheRequest(lod, p));
                }
            }
        }
    }
}

void QuadTerrain::
discardSubTree(GlData* glData, Node* node)
{
    if (node->children==NULL)
        return;

    //recurse down the children that have subtrees
    for (uint i=0; i<4; ++i)
    {
        if (node->children[i].children != NULL)
            discardSubTree(glData, &node->children[i]);
    }
    //return the children to the pool
    glData->releaseNodeBlock(node->children);
    node->children = NULL;
}

void QuadTerrain::
traverse(GLContextData& contextData, GlData* glData, Node* node,
         CacheRequests& requests)
{
    //recurse to the active nodes
    for (uint i=0; node->children!=NULL && i<4; ++i)
        traverse(contextData, glData, &(node->children[i]), requests);

    if (node->children == NULL)
    {
    //- evaluate merge, only if we're not already at the root
        if (node->index.level != 0)
        {
            float parentVisible =
                glData->visibility.evaluate(node->parent);
            float parentLod    = glData->lod.evaluate(node->parent);
            if (!parentVisible || parentLod<1.0)
            {
                if (merge(glData, node, parentLod, requests))
                    return;
            }
        }

    //- evaluate node for splitting
        //compute visibility and lod
        float visible = glData->visibility.evaluate(node);
        float lod     = glData->lod.evaluate(node);
        if (visible && lod>1.0)
        {
            if (split(glData, node, lod, requests))
            {
                //continue traversal in the children
                for (uint i=0; i<4; ++i)
                {
                    traverse(contextData,glData,&(node->children[i]),requests);
                }
                return;
            }
        }

    //- draw the node
        //confirm current state of the node
        confirmCurrent(node, lod, requests);
        //draw only the visible
        if (visible)
            drawNode(contextData, glData, node);
        //non-visible lose their hold on cached video data
        else
            node->videoBuffer = NULL;
    }
}

void QuadTerrain::
findNodeData(Node* node, Node*& dem, Node*& color, DataCropOut& crop)
{
    //find dem data
    crop.demScale     = 1.0f;
    crop.demOffset[0] = crop.demOffset[1] = 0.0f;
    dem = node;
    while (dem->demTile==DemFile::INVALID_TILEINDEX || dem->mainBuffer==NULL)
    {
        //update the crop region
        crop.demScale *= 0.5f;
        crop.demOffset[0] += dem->index.child&0x1 ? crop.demScale : 0;
        crop.demOffset[1] += dem->index.child&0x2 ? crop.demScale : 0;
        //move up to the parent
        dem = dem->parent;
    }

    //find color data
    crop.colorScale     = 1.0f;
    crop.colorOffset[0] = crop.colorOffset[1] = 0.0f;
    color = node;
    while (color->colorTile==ColorFile::INVALID_TILEINDEX ||
           color->mainBuffer==NULL)
    {
        //update the crop region
        crop.colorScale *= 0.5f;
        crop.colorOffset[0] += color->index.child&0x1 ? crop.colorScale : 0;
        crop.colorOffset[1] += color->index.child&0x2 ? crop.colorScale : 0;
        //move up to the parent
        color = color->parent;
    }
}

QuadNodeVideoDataRef QuadTerrain::
prepareGlData(GlData* glData, Node* node, DataCropOut& crop)
{
///\todo HACK OF DOOM! scavenge for data to use!!
#if 1
    QuadNodeVideoDataRef res;

    //find out which data we want to use and stream it
    Node* demNode;
    Node* colorNode;
    findNodeData(node, demNode, colorNode, crop);

    //we need our own buffer, at the very least for the unique geometry
    bool existed;
    VideoCacheBuffer* videoBuf = glData->videoCache.getBuffer(node->index,
                                                              &existed);
    if (videoBuf == NULL)
        videoBuf = glData->videoCache.getStreamBuffer();
    else
        videoBuf->touch();
    const QuadNodeVideoData& videoData = videoBuf->getData();

    const QuadNodeMainData&  mainData = node->mainBuffer->getData();
    //geometry
    res.geometry = videoData.geometry;
    if (!existed)
    {
        glBindTexture(GL_TEXTURE_2D, res.geometry);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        TILE_RESOLUTION, TILE_RESOLUTION, GL_RGB, GL_FLOAT,
                        mainData.geometry);
    }

    //elevation
    res.height = videoData.height;
    if (demNode==node)
    {
        if (!existed)
        {
            glBindTexture(GL_TEXTURE_2D, res.height);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                            TILE_RESOLUTION, TILE_RESOLUTION, GL_RED, GL_FLOAT,
                            mainData.height);
        }
    }
    else
    {
        //try to find a video cached source
        VideoCacheBuffer* demVideoBuf =
            glData->videoCache.findCached(demNode->index);
        if (demVideoBuf != NULL)
        {
            res.height = demVideoBuf->getData().height;
            demVideoBuf->touch();
        }
        else
        {
            demNode->mainBuffer->touch();
            const QuadNodeMainData&  demMainData =
                demNode->mainBuffer->getData();
            glBindTexture(GL_TEXTURE_2D, res.height);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                            TILE_RESOLUTION, TILE_RESOLUTION, GL_RED, GL_FLOAT,
                            demMainData.height);
        }
    }

    //color
    res.color = videoData.color;
    if (colorNode==node)
    {
        if (!existed)
        {
            glBindTexture(GL_TEXTURE_2D, res.color);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                            TILE_RESOLUTION, TILE_RESOLUTION, GL_RGB,
                            GL_UNSIGNED_BYTE, mainData.color);
        }
    }
    else
    {
        //try to find a video cached source
        VideoCacheBuffer* colorVideoBuf =
            glData->videoCache.findCached(colorNode->index);
        if (colorVideoBuf != NULL)
        {
            res.color = colorVideoBuf->getData().color;
            colorVideoBuf->touch();
        }
        else
        {
            colorNode->mainBuffer->touch();
            const QuadNodeMainData&  colorMainData =
                colorNode->mainBuffer->getData();
            glBindTexture(GL_TEXTURE_2D, res.color);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                            TILE_RESOLUTION, TILE_RESOLUTION, GL_RGB,
                            GL_UNSIGNED_BYTE, colorMainData.color);
        }
    }

    return res;
#else
    //check if we already have the data locally cached
    if (node->videoBuffer != NULL)
    {
///\todo debug
VideoCacheBuffer* shouldBeBuf = glData->videoCache.findCached(node->index);
if (shouldBeBuf != node->videoBuffer)
    std::cout << "FRAKAWE";

        node->videoBuffer->touch();
        return node->videoBuffer->getData();
    }

    bool existed;
    VideoCacheBuffer* videoBuf = glData->videoCache.getBuffer(node->index,
                                                              &existed);
///\todo debug
checkTree(glData, glData->root);
    if (existed)
    {
        //if there was already a match in the cache, just use that data
        node->videoBuffer = videoBuf;
        node->videoBuffer->touch();
        return node->videoBuffer->getData();
    }
    else
    {
        //in any case the data has to be transfered from main memory
        VideoCacheBuffer* xferBuf = NULL;
        if (videoBuf)
        {
            //if we can use a stable buffer, then associate it with the node
            node->videoBuffer = videoBuf;
            node->videoBuffer->touch();
            xferBuf = videoBuf;
        }
        else
        {
            //the cache couldn't provide a slot, use the streamBuffer
            xferBuf = glData->videoCache.getStreamBuffer();
        }

        const QuadNodeMainData&  mainData  = node->mainBuffer->getData();
        const QuadNodeVideoData& videoData = xferBuf->getData();
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
    DataCropOut crop;
    QuadNodeVideoDataRef data = prepareGlData(glData, node, crop);

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

//    glPointSize(1.0);
//    glDrawRangeElements(GL_POINTS, 0,
    glUniform3f(glData->centroidUniform,
                node->centroid[0], node->centroid[1], node->centroid[2]);

    glUniform1f(glData->demScaleUniform,  crop.demScale);
    glUniform2f(glData->demOffsetUniform, crop.demOffset[0], crop.demOffset[1]);

    glUniform1f(glData->colorScaleUniform, crop.colorScale);
    glUniform2f(glData->colorOffsetUniform,
                crop.colorOffset[0], crop.colorOffset[1]);

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


QuadTerrain::GlData::
GlData(QuadTerrain* terrain, const TreeIndex& iRootIndex,
       MainCacheBuffer* iRootBuffer, const Scope& baseScope,
       VideoCache& iVideoCache) :
    root(NULL), videoCache(iVideoCache), verticalScale(1.0),
    vertexAttributeTemplate(0), indexTemplate(0), verticalScaleUniform(0),
    centroidUniform(0), demScaleUniform(0), demOffsetUniform(0),
    colorScaleUniform(0), colorOffsetUniform(0)
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
    root->demTile    = 0;

    //generate/fetch the data content for the static data
    const QuadNodeMainData& rootData = root->mainBuffer->getData();

    if (root->terrain->demFile != NULL)
    {
        root->terrain->demNodata =
            root->terrain->demFile->getDefaultPixelValue();
    }
    else
        root->terrain->demNodata = DemHeight(-1);
    root->terrain->sourceDem(root, rootData.height);

/**\todo the geometry generation is now dependent on the data!! Need to load it
in before the geometry can be generated. So long as the data is not loaded yet
use the values of the node from which the data is being sampled */
    root->terrain->generateGeometry(root, rootData.geometry);

    root->colorTile = 0;
    if (root->terrain->colorFile != NULL)
    {
        root->terrain->colorNodata =
            root->terrain->colorFile->getDefaultPixelValue();
    }
    else
        root->terrain->colorNodata = TextureColor(255,255,255);
    root->terrain->sourceColor(root, rootData.color);

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

    demScaleUniform    = shader.getUniformLocation("demScale");
    demOffsetUniform   = shader.getUniformLocation("demOffset");
    colorScaleUniform  = shader.getUniformLocation("colorScale");
    colorOffsetUniform = shader.getUniformLocation("colorOffset");

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
    QuadTerrain::discardSubTree(this, root);
    delete root;

    for (NodeBlocks::iterator it=nodePool.begin(); it!=nodePool.end(); ++it)
        delete[] *it;

    if (vertexAttributeTemplate != 0)
        glDeleteBuffers(1, &vertexAttributeTemplate);
    if (indexTemplate != 0)
        glDeleteBuffers(1, &indexTemplate);
}

Node* QuadTerrain::GlData::
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

void QuadTerrain::GlData::
releaseNodeBlock(Node* children)
{
///\todo control size of dangling buffers
    nodePool.push_back(children);
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
    delete positionsInMemory;
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
    delete indicesInMemory;
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
