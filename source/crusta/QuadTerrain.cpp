#include <crusta/QuadTerrain.h>

#include <algorithm>
#include <assert.h>

#include <crusta/QuadCache.h>
///\todo remove
#include <crusta/simplexnoise1234.h>

BEGIN_CRUSTA

static const uint NUM_GEOMETRY_INDICES =
    (TILE_RESOLUTION-1)*(TILE_RESOLUTION*2 + 2) - 2;
static const float TEXTURE_COORD_STEP  = 1.0 / TILE_RESOLUTION;
static const float TEXTURE_COORD_START = TEXTURE_COORD_STEP * 0.5;
static const float TEXTURE_COORD_END   = 1.0 - TEXTURE_COORD_START;

template <class PointParam>
inline
PointParam
toSphere(const PointParam& p)
{
    typedef typename PointParam::Scalar Scalar;
    
    double len = Geometry::mag(p);
    return PointParam(Scalar(p[0]/len), Scalar(p[1]/len), Scalar(p[2]/len));
}
static void
mid(uint oneIndex, uint twoIndex, QuadNodeMainData::Vertex* vertices)
{
    QuadNodeMainData::Vertex& one = vertices[oneIndex];
    QuadNodeMainData::Vertex& two = vertices[twoIndex];
    QuadNodeMainData::Vertex& res = vertices[(oneIndex + twoIndex) >> 1];
    
    res.position[0] = (one.position[0]  + two.position[0] ) * 0.5f;
    res.position[1] = (one.position[1]  + two.position[1] ) * 0.5f;
    res.position[2] = (one.position[2]  + two.position[2] ) * 0.5f;

    double invLen   = 1.0 / sqrt(res.position[0]*res.position[0] +
                                 res.position[1]*res.position[1] +
                                 res.position[2]*res.position[2]);
    res.position[0] *= invLen;
    res.position[1] *= invLen;
    res.position[2] *= invLen;
}
static void
centroid(uint oneIndex, uint twoIndex, uint threeIndex, uint fourIndex,
         QuadNodeMainData::Vertex* vertices)
{
    QuadNodeMainData::Vertex& one   = vertices[oneIndex];
    QuadNodeMainData::Vertex& two   = vertices[twoIndex];
    QuadNodeMainData::Vertex& three = vertices[threeIndex];
    QuadNodeMainData::Vertex& four  = vertices[fourIndex];
    QuadNodeMainData::Vertex& res   = vertices[(oneIndex + twoIndex +
                                       threeIndex+fourIndex) >> 2];
    
    res.position[0] = (one.position[0]   + two.position[0] +
                       three.position[0] + four.position[0]) * 0.25f;
    res.position[1] = (one.position[1]   + two.position[1] +
                       three.position[1] + four.position[1]) * 0.25f;
    res.position[2] = (one.position[2]   + two.position[2] +
                       three.position[2] + four.position[2]) * 0.25f;

    double invLen   = 1.0 / sqrt(res.position[0]*res.position[0] +
                                 res.position[1]*res.position[1] +
                                 res.position[2]*res.position[2]);
    res.position[0] *= invLen;
    res.position[1] *= invLen;
    res.position[2] *= invLen;
}



QuadTerrain::
QuadTerrain(uint8 patch, const Scope& scope) :
    basePatchId(patch), baseScope(scope)
{
    //create root data and pin it in the cache
    TreeIndex rootIndex(patch);
    MainCacheBuffer* rootBuffer =
        crustaQuadCache.getMainCache().getBuffer(rootIndex);
    assert(rootBuffer!=NULL);
    rootBuffer->pin();

    //generate/fetch the data content for the static data
    const QuadNodeMainData& rootData = rootBuffer->getData();
    generateGeometry(scope, rootData.geometry);
    generateHeight(rootData.geometry, rootData.height);
    generateColor(rootData.height, rootData.color);
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

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);

    //setup the evaluators
    glData->visibility.frustum.setFromGL();
    glData->lod.frustum = glData->visibility.frustum;

    /* display could be multi-threaded. Buffer all the node data requests and
       merge them into the request list en block */
    MainCacheRequests dataRequests;

    /* traverse the terrain tree, update as necessary and issue drawing commands
       for active nodes */
checkTree(glData, glData->root);
    traverse(glData, glData->root, dataRequests);
checkTree(glData, glData->root);

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
generateGeometry(const Scope& scope, QuadNodeMainData::Vertex* vertices)
{
    //set the initial four corners from the scope
    static const uint cornerIndices[] = {
        0,
        TILE_RESOLUTION-1,
        (TILE_RESOLUTION-1)*TILE_RESOLUTION,
        (TILE_RESOLUTION-1)*TILE_RESOLUTION + (TILE_RESOLUTION-1)
    };
    for (uint i=0; i<4; ++i)
    {
        vertices[cornerIndices[i]] = QuadNodeMainData::Vertex(
            QuadNodeMainData::Vertex::Position(scope.corners[i][0],
                                               scope.corners[i][1],
                                               scope.corners[i][2]));
    }
    
    //refine the rest
    static const uint endIndex  = TILE_RESOLUTION * TILE_RESOLUTION;
    for (uint blockSize=TILE_RESOLUTION-1; blockSize>1; blockSize>>=1)
    {
        uint blockSizeY = blockSize * TILE_RESOLUTION;
        for (uint startY=0, endY=blockSizeY; endY<endIndex;
             startY+=blockSizeY, endY+=blockSizeY)
        {
            for (uint startX=0, endX=blockSize; endX<TILE_RESOLUTION;
                 startX+=blockSize, endX+=blockSize)
            {
                //left mid point (only if on the edge where it has not already
                //been computed by the neighbor)
                if (startX == 0)
                    mid(startY, endY, vertices);                    
                //bottom mid point
                mid(endY+startX, endY+endX, vertices);
                //right mid point
                mid(endY+endX, startY+endX, vertices);
                //top mid point (only if on the edge where it has not already
                //been computed by the neighbor)
                if (startY == 0)
                    mid(endX, startX, vertices);
                //centroid
                centroid(startY+startX, endY+startX, endY+endX, startY+endX,
                         vertices);
            }
        }
    }
}

void QuadTerrain::
generateHeight(QuadNodeMainData::Vertex* vertices, float* heights)
{
    uint numHeights = TILE_RESOLUTION*TILE_RESOLUTION;
    float* end = heights + numHeights;
    for (; heights<end; ++heights, ++vertices)
    {
        float theta = acos(vertices->position[2]);
        float phi = atan2(vertices->position[1], vertices->position[0]);
        *heights = SimplexNoise1234::noise(theta, phi);
        *heights += 1.0f;
        *heights /= 8.0f;
        *heights += 1.0f;
    }
}

void QuadTerrain::
generateColor(float* heights, uint8* colors)
{
    static const uint  PALETTE_SIZE = 3;
    static const float PALETTE_BUCKET_SIZE = 1.0f / (PALETTE_SIZE-1);
    static const float heightPalette[PALETTE_SIZE][3] = {
        {0.85f, 0.83f, 0.66f}, {0.80f, 0.21f, 0.28f}, {0.29f, 0.37f, 0.50f} };

    uint numHeights = TILE_RESOLUTION*TILE_RESOLUTION;
    float* end = heights + numHeights;
    for (; heights<end; ++heights, colors+=3)
    {
        float alpha = (*heights - 1.0f) * 4.0f;
        alpha /= PALETTE_BUCKET_SIZE;
        uint low  = (uint)(alpha);
        uint high = low==(PALETTE_SIZE-1) ? (PALETTE_SIZE-1) : low+1;
        alpha -= low;
        
        colors[0] = ((1.0f-alpha)*heightPalette[low][0] +
                    alpha*heightPalette[high][0]) * 255;
        colors[1] = ((1.0f-alpha)*heightPalette[low][1] +
                    alpha*heightPalette[high][1]) * 255;
        colors[2] = ((1.0f-alpha)*heightPalette[low][2] +
                    alpha*heightPalette[high][2]) * 255;
    }
}


QuadTerrain::Node::
Node() :
    mainBuffer(NULL), videoBuffer(NULL), parent(NULL), children(NULL)
{
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


void QuadTerrain::Node::
discardSubTree(GlData* glData)
{
    if (children==NULL)
        return;

    //recurse down the children that have subtrees
    for (uint i=0; i<4; ++i)
    {
        if (children[i].children != NULL)
            children[i].discardSubTree(glData);
    }
    //return the children to the pool
    glData->releaseNodeBlock(children);
    children = NULL;
}


bool QuadTerrain::
merge(GlData* glData, Node* node, float lod,
      MainCacheRequests& requests)
{
    //if we don't have the parent's data then we must request it
    node->parent->mainBuffer =
        crustaQuadCache.getMainCache().findCached(node->parent->index);
    if (node->parent->mainBuffer == NULL)
    {
        requests.push_back(MainCacheRequest(lod, node->parent->index,
                                            node->parent->scope));
        return false;
    }

    //make the parent current
    node->parent->videoBuffer = NULL;
DEBUG_OUT(1, "-Merge: %s:\n", node->parent->index.str().c_str());
    node->parent->discardSubTree(glData);
printTree(glData->root); DEBUG_OUT(1, "\n\n");
checkTree(glData, glData->root);
    return true;
}

void QuadTerrain::
computeChildScopes(Node* node)
{
    assert(node->children!=NULL);
    static const uint midIndices[8] = {
        Scope::LOWER_LEFT,  Scope::LOWER_RIGHT,
        Scope::UPPER_LEFT,  Scope::LOWER_LEFT,
        Scope::UPPER_RIGHT, Scope::LOWER_RIGHT,
        Scope::UPPER_LEFT,  Scope::UPPER_RIGHT
    };
    
    const Point* corners = node->scope.corners;
    
    Point mids[4];
    for (uint i=0, j=0; i<4; ++i)
    {
        const Point& c1 = corners[midIndices[j++]];
        const Point& c2 = corners[midIndices[j++]];
        mids[i] = Geometry::mid(c1, c2);
        mids[i] = toSphere(mids[i]);
    }
    
    Point center =
        Point((corners[0][0]+corners[1][0]+corners[2][0]+corners[3][0])/4.0,
              (corners[0][1]+corners[1][1]+corners[2][1]+corners[3][1])/4.0,
              (corners[0][2]+corners[1][2]+corners[2][2]+corners[3][2])/4.0);
    center = toSphere(center);
    
    const Point* newCorners[16] = {
        &corners[Scope::LOWER_LEFT], &mids[0], &mids[1], &center,
        &mids[0], &corners[Scope::LOWER_RIGHT], &center, &mids[2],
        &mids[1], &center, &corners[Scope::UPPER_LEFT], &mids[3],
        &center, &mids[2], &mids[3], &corners[Scope::UPPER_RIGHT]
    };
    for (uint i=0, j=0; i<4; ++i)
    {
        for (uint k=0; k<4; ++k, ++j)
            node->children[i].scope.corners[k] = (*newCorners[j]);
    }
}

bool QuadTerrain::
split(GlData* glData, Node* node, float lod, MainCacheRequests& requests)
{
    //tentatively assemble the children
    node->children = glData->grabNodeBlock();
    computeChildScopes(node);

    //verify that the critical data of the children is cached
    bool allChildrenCached = true;
    for (uint i=0; i<4; ++i)
    {
        Node& child = node->children[i];
        child.index = node->index.down(i);
        child.mainBuffer =
            crustaQuadCache.getMainCache().findCached(child.index);
        if (child.mainBuffer == NULL)
        {
            requests.push_back(MainCacheRequest(lod, child.index, child.scope));
            allChildrenCached = false;
        }
        child.videoBuffer = NULL;
        child.parent = node;
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
        /* split was successful, invalidate the buffers of the current node,
           since they might be reused for another node */
        node->mainBuffer  = NULL;
        node->videoBuffer = NULL;
DEBUG_OUT(1, "+Split: %s:\n", node->index.str().c_str());
printTree(glData->root); DEBUG_OUT(1, "\n\n");
checkTree(glData, glData->root);
        return true;
    }
}


void QuadTerrain::
confirmCurrent(Node* node, float lod, MainCacheRequests& requests)
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
            requests.push_back(MainCacheRequest(lod, p->index, p->scope));

        if (node->parent->parent != NULL)
        {
            //touch/request parent's parent
            p = node->parent->parent;
            if (p->mainBuffer != NULL)
                p->mainBuffer->touch();
            else
                requests.push_back(MainCacheRequest(lod, p->index, p->scope));
        }
    }
}

void QuadTerrain::
traverse(GlData* glData, Node* node, MainCacheRequests& requests)
{
    //recurse to the active nodes
    for (uint i=0; node->children!=NULL && i<4; ++i)
        traverse(glData, &(node->children[i]), requests);

    if (node->children == NULL)
    {
    //- evaluate merge, only if we're not already at the root
        if (false)//node->index.level != 0)
        {
            float parentVisible =
                glData->visibility.evaluate(node->parent->scope);
            float parentLod    = glData->lod.evaluate(node->parent->scope);
            if (!parentVisible || parentLod<-1.0)
            {
                if (merge(glData, node, parentLod, requests))
                    return;
            }
        }

    //- evaluate node for splitting
        //compute visibility and lod
        float visible = glData->visibility.evaluate(node->scope);
        float lod     = glData->lod.evaluate(node->scope);
        if (false)//visible && lod>1.0)
        {
            if (split(glData, node, lod, requests))
            {
                //continue traversal in the children
                for (uint i=0; i<4; ++i)
                {
                    traverse(glData, &(node->children[i]), requests);
                }
                return;
            }
        }
        
    //- draw the node
        //confirm current state of the node
        confirmCurrent(node, lod, requests);
        //draw only the visible
        if (visible)
            drawNode(glData, node);
        //non-visible lose their hold on cached video data
        else
            node->videoBuffer = NULL;
    }
}

const QuadNodeVideoData& QuadTerrain::
prepareGlData(GlData* glData, Node* node)
{
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
}

void QuadTerrain::
drawNode(GlData* glData, Node* node)
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
    glDrawRangeElements(GL_TRIANGLE_STRIP, 0,
                        (TILE_RESOLUTION*TILE_RESOLUTION) - 1,
                        NUM_GEOMETRY_INDICES, GL_UNSIGNED_SHORT, 0);
#endif

#if 0
glData->shader.disablePrograms();
    Point* c = node->scope.corners;
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

#if 0
glData->shader.disablePrograms();
    Point* c = node->scope.corners;
    glDisable(GL_LIGHTING);
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
    glEnable(GL_TEXTURE_2D);
glData->shader.useProgram();
#endif
}


QuadTerrain::GlData::
GlData(const TreeIndex& iRootIndex, MainCacheBuffer* iRootBuffer,
       const Scope& baseScope, VideoCache& iVideoCache) :
    root(NULL), videoCache(iVideoCache), vertexAttributeTemplate(0),
    indexTemplate(0)
{
    //initialize the static gl buffer templates
    generateVertexAttributeTemplate();
    generateIndexTemplate();
    
    //initialize the active front to be just the root node
    root = new Node;
    root->index      = iRootIndex;
    root->scope      = baseScope;
    root->mainBuffer = iRootBuffer;
    
    //initialize the shader
//    shader.compileVertexShader("elevation.vs");
    shader.compileVertexShader("litElevation.vs");
    shader.compileFragmentShader("elevation.fs");
    shader.linkShader();
    shader.useProgram();
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
    lod.bias = -1.0;
}

QuadTerrain::GlData::
~GlData()
{
    root->discardSubTree(this);
    delete root;

    for (NodeBlocks::iterator it=nodePool.begin(); it!=nodePool.end(); ++it)
        delete[] *it;

    if (vertexAttributeTemplate != 0)
        glDeleteBuffers(1, &vertexAttributeTemplate);
    if (indexTemplate != 0)
        glDeleteBuffers(1, &indexTemplate);
}

QuadTerrain::Node* QuadTerrain::GlData::
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
    GlData* glData = new GlData(rootIndex, rootBuffer, baseScope,
                                crustaQuadCache.getVideoCache(contextData));
    //commit the context data
    contextData.addDataItem(this, glData);
}


END_CRUSTA
