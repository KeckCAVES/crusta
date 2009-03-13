#include <QuadTerrain.h>

#include <assert.h>

#include <QuadCache.h>
///\todo remove
#include <simplexnoise1234.h>

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
    basePatchId(patch), baseScope(scope), frameNumber(0)
{
    //create root data and pin it in the cache
    TreeIndex rootIndex(patch);
    MainCacheBuffer* rootBuffer =
        crustaQuadCache.getMainCache().getBuffer(rootIndex);
    rootBuffer->pin();

    //generate/fetch the data content for the static data
    const QuadNodeMainData& rootData = rootBuffer->getData();
    generateGeometry(scope, rootData.geometry);
generateHeights(rootData.geometry, rootData.height);
///\todo color data
}

void QuadTerrain::
frame()
{
    crustaQuadCache.getMainCache().frame();
    ++frameNumber;
}

void QuadTerrain::
display(GLContextData& contextData)
{
    //notify the video cache of a new frame
    crustaQuadCache.getVideoCache(contextData).frame();

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
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1.0f);
    
    glData->shader.useProgram();

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);
    /* display could be multi-threaded. Buffer all the node data requests and
       merge them into the request list en block */
    MainCacheRequests dataRequests;

    ActiveList& activeList = glData->activeList;
    /* first pass: evaluate active nodes, apply modifications that have their
       data already cached and request uncached data. The first pass is
       necessary, because merging is handled by being prompted by a child node
       being represented to finely. This means that a sibling node already
       traversed might need to be deactivated and musn't have had its drawing
       commands issued yet. Requiring a prompt from a child vs. simply checking
       the parent is for the purpose of lazy fetching of parent data: it is
       unlikely that the parent is evaluated as being a valid approximation if
       none of the children are close to needing to be merged.
     */
    for (ActiveList::iterator it=activeList.begin(); it!=activeList.end(); ++it)
        evaluateActive(activeList, it, dataRequests);

    /* second pass: confirm activity and issue drawing commands */
    for (ActiveList::iterator it=activeList.begin(); it!=activeList.end(); ++it)
    {
        //confirm activity of the node
        it->mainBuffer->touch(frameNumber);
        //issue drawing commands
        drawActive(it, glData, contextData);        
    }

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
generateHeights(QuadNodeMainData::Vertex* vertices, float* heights)
{
    uint numHeights = TILE_RESOLUTION*TILE_RESOLUTION;
    float* tex = new float[numHeights];
    for (float* cur=tex; cur<tex+numHeights; ++cur, ++vertices)
    {
        float theta = acos(vertices->position[2]);
        float phi = atan2(vertices->position[1], vertices->position[0]);
        float elevation = SimplexNoise1234::noise(theta, phi);
        elevation += 1.0f;
        elevation /= 8.0f;
        elevation += 1.0f;
        *cur = elevation;
    }
}



QuadTerrain::ActiveNode::
ActiveNode() :
    mainBuffer(NULL), videoBuffer(NULL), visible(false)
{
}
QuadTerrain::ActiveNode::
ActiveNode(const TreeIndex& iIndex, MainCacheBuffer* iBuffer,
           const Scope& iScope, bool iVisible) :
    index(iIndex), mainBuffer(iBuffer), videoBuffer(NULL), scope(iScope),
    visible(iVisible)
{
}

void QuadTerrain::
computeParentScope(const ActiveList& activeList,
                   const ActiveList::iterator& curNode)
{
    TreeIndex parentIndex = curNode->index.up();

    //we need to combine the subtree scopes into the parent's 
    Scope& parentScope = curNode->parentScope;
    
    /* build a mask to separate the index into the path to parent part and the
     subtree part */
    uint32 mask = 0x0;
    for (uint i=1; i<parentIndex.level; ++i, mask<<=2)
        mask |= 0x3;
    //mask out the subtree from the parent index
    uint32 toParent = parentIndex.index & mask;
    
    //determine the "left end" of the subtree
    ActiveList::const_iterator start = curNode;
    if (start != activeList.begin())
    {
        bool reachedBegin = false;
        for (--start; ((start->index.index)&mask) == toParent; --start)
        {
            if (start == activeList.begin())
            {
                reachedBegin = true;
                break;
            }
        }
        if (!reachedBegin)
            ++start; //because now we're one too far
    }
    //determine the "right end" of the subtree
    ActiveList::const_iterator end = curNode;
    for (++end; ((end->index.index)&mask) == toParent && end!=activeList.end();
         ++end) ;

    //parse out the parentScope from the subtree
    for (ActiveList::const_iterator it=start; it!=end; ++it)
    {
        //compute the residual path (including parent)
        uint32 toNode = it->index.index >> ((parentIndex.level-1)*2);
        //verify that the path to the node is of a corner of the parent
        bool  isCorner = true;
        uint8 corner   = toNode&0x3;
        toNode >>= 2;
        for (uint i=1; i<(it->index.level - parentIndex.level);
             ++i, toNode>>=2)
        {
            if (corner != (toNode & 0x3))
            {
                isCorner = false;
                break;
            }
        }
        //set the corner value
        if (isCorner)
            parentScope.corners[corner] = it->scope.corners[corner];
    }
}

void QuadTerrain::
computeChildScopes(const ActiveList::iterator& curNode)
{
    static const uint midIndices[8] = {
        Scope::LOWER_LEFT,  Scope::LOWER_RIGHT,
        Scope::UPPER_LEFT,  Scope::LOWER_LEFT,
        Scope::UPPER_RIGHT, Scope::LOWER_RIGHT,
        Scope::UPPER_LEFT,  Scope::UPPER_RIGHT
    };
    
    const Point* corners = curNode->scope.corners;
    
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
            curNode->childScopes[i].corners[k] = (*newCorners[j]);
    }
#if 0
    static const uint midIndices[8] = {
        Scope::LOWER_LEFT,  Scope::LOWER_RIGHT,
        Scope::UPPER_LEFT,  Scope::LOWER_LEFT,
        Scope::UPPER_RIGHT, Scope::LOWER_RIGHT,
        Scope::UPPER_LEFT,  Scope::UPPER_RIGHT
    };
    static const uint childToMid[8] = { 0, 1, 0, 2, 1, 3, 2, 3 };

    //compute the scope of the children
    const Point* corners = curNode->buffer->getData()->scope.corners;
    
    Point mids[2];
    for (uint i=0, j=childToMid[child*2]; i<2; ++i, j=childToMid[child*2 + i])
    {
        const Point& c1 = corners[midIndices[j]];
        const Point& c2 = corners[midIndices[j]];
        mids[i] = Geometry::mid(c1, c2);
        mids[i] = toSphere(mids[i]);
    }
    
    Point center = centroid(corners[0], corners[1], corners[2], corners[3]);
    center = toSphere(center);
    
    const Point* newCorners[16] = {
        &corners[Scope::LOWER_LEFT], &mids[0], &mids[1], &center,
        &mids[0], &corners[Scope::LOWER_RIGHT], &center, &mids[1],
        &mids[0], &center, &corners[Scope::UPPER_LEFT], &mids[1],
        &center, &mids[0], &mids[1], &corners[Scope::UPPER_RIGHT]
    };

    Scope childScope;
    for (uint i=0, j=newCorners[i*4]; i<4; ++i, ++j)
        childScope.corners[i] = (*newCorners[j]);
    
    return childScope;
#endif
}

void QuadTerrain::
mergeInList(ActiveList& activeList, ActiveList::iterator& curNode,
            const ActiveNode& parentNode)
{
    /* build a mask to separate the index into the path to parent part and the
       subtree part */
    uint32 mask = 0x0;
    for (uint i=0; i<parentNode.index.level; ++i, mask<<=2)
        mask |= 0x3;
    //mask out the subtree from the parent index
    uint32 toParent = parentNode.index.index & mask;

    //determine the "left end" of the subtree
    ActiveList::iterator start = curNode;
    if (start != activeList.begin())
    {
        bool reachedBegin = false;
        for (--start; ((start->index.index)&mask) == toParent; --start)
        {
            if (start == activeList.begin())
            {
                reachedBegin = true;
                break;
            }
        }
        if (!reachedBegin)
            ++start; //because now we're one too far
    }
    //determine the "right end" of the subtree
    ActiveList::iterator end = curNode;
    for (++end; ((end->index.index)&mask) == toParent && end!=activeList.end();
         ++end) ;
    
    //just get rid of the subtree
    curNode = activeList.erase(start, end);    
    //insert the parent into the list
    curNode = activeList.insert(curNode, parentNode);
    computeParentScope(activeList, curNode);
    computeChildScopes(curNode);
}

void QuadTerrain::
merge(ActiveList& activeList, ActiveList::iterator& it, float lod,
      MainCacheRequests& requests)
{
    TreeIndex parentIndex = it->index.up();
    
    //evaluate the parent's visibility and lod
    bool  parentVisible = visibilityEvaluator.evaluate(it->parentScope);
    float parentLod     = lodEvaluator.evaluate(it->parentScope);
    //perform the merge if it agrees with the parent's evaluation
    if (!parentVisible || parentLod<-1.0)
    {
        //if we don't have the parent's data then we must request it
        MainCacheBuffer* parentBuffer =
            crustaQuadCache.getMainCache().findCached(parentIndex);
        if (parentBuffer == NULL)
        {
            requests.push_back(MainCacheRequest(lod, parentIndex,
                                                it->parentScope));
            return;
        }

        //replace the parent's subtree with the parent in the active list
        mergeInList(activeList, it, ActiveNode(parentIndex, parentBuffer,
                                               it->parentScope, parentVisible));
    }
}

void QuadTerrain::
split(ActiveList& activeList, ActiveList::iterator& it, float lod,
      MainCacheRequests& requests)
{
    //verify that the critical data of the children is cached
    ActiveNode children[4];
    bool allChildrenCached = true;
    for (uint i=0; i<4; ++i)
    {
        ActiveNode& child = children[i];
        child.index = it->index.down(i);
        child.mainBuffer =
            crustaQuadCache.getMainCache().findCached(child.index);
        if (child.mainBuffer == NULL)
        {
            requests.push_back(MainCacheRequest(lod, child.index,
                                                it->childScopes[i]));
            allChildrenCached = false;
        }
        child.parentScope = it->scope;
        child.scope       = it->childScopes[i];
    }
    if (!allChildrenCached)
        return;
   
    //replace the parent with the children in the active list
    it = activeList.erase(it);
    for (int i=3; i>=0; --i)
    {
        it = activeList.insert(it, children[i]);
        computeChildScopes(it);
    }
}


void QuadTerrain::
evaluateActive(ActiveList& activeList, ActiveList::iterator& it,
               MainCacheRequests& requests)
{
    //update the visibility and lod
    it->visible = visibilityEvaluator.evaluate(it->scope);
    float lod = lodEvaluator.evaluate(it->scope);
    
//- evaluate merge, only if we're not already at the root
    if (it->index.level!=0 && (!it->visible || lod<-1.0))
    {
        merge(activeList, it, lod, requests);
        return;
    }

//- evaluate node for splitting
    if (it->visible && lod>1.0)
        split(activeList, it, lod, requests);
}

const QuadNodeVideoData& QuadTerrain::
prepareGLData(const ActiveList::iterator it, GLContextData& contextData)
{
    //check if we already have the data locally cached
    if (it->videoBuffer != NULL)
    {
        return it->videoBuffer->getData();
    }

    //get a buffer from
    VideoCache& videoCache = crustaQuadCache.getVideoCache(contextData);
    
    bool existed;
    VideoCacheBuffer* videoBuf = videoCache.getBuffer(it->index, existed);
    if (existed)
    {
        //if there was already a match in the cache, just use that data
        it->videoBuffer = videoBuf;
        return it->videoBuffer->getData();
    }
    else
    {
        //in any case the data has to be transfered from main memory
        VideoCacheBuffer* xferBuf = NULL;
        if (videoBuf)
        {
            //if we can use a stable buffer, then associate it with the node
            it->videoBuffer = videoBuf;
            xferBuf = videoBuf;
        }
        else
        {
            //the cache couldn't provide a slot, use the streamBuffer
            xferBuf = videoCache.getStreamBuffer();
        }

        const QuadNodeMainData&  mainData  = it->mainBuffer->getData();
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
        
        //return the data
        return videoData;
    }
}

void QuadTerrain::
drawActive(const ActiveList::iterator& it, GlData* glData,
           GLContextData& contextData)
{
///\todo accommodate for lazy data fetching
    const QuadNodeVideoData& data = prepareGLData(it, contextData);

    glActiveTexture(GL_TEXTURE0); 
    glBindTexture(GL_TEXTURE_2D, data.geometry);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, data.height);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, data.color);
    
    glBindBuffer(GL_ARRAY_BUFFER,         glData->vertexAttributeTemplate);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glData->indexTemplate);

    glVertexPointer(2, GL_FLOAT, 0, 0);
//    glIndexPointer(GL_UNSIGNED_SHORT, 0, 0);

    glDrawRangeElements(GL_TRIANGLE_STRIP, 0,
                        (TILE_RESOLUTION*TILE_RESOLUTION) - 1,
                        NUM_GEOMETRY_INDICES, GL_UNSIGNED_SHORT, 0);
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
    for (uint b=0; b<TILE_RESOLUTION+1; ++b, inc=-inc, alt=1-alt,
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
    GlData* glData = new GlData(rootIndex, rootBuffer, baseScope);
    //commit the context data
    contextData.addDataItem(this, glData);
}

QuadTerrain::GlData::
GlData(const TreeIndex& iRootIndex, MainCacheBuffer* iRootBuffer,
       const Scope& baseScope) :
    vertexAttributeTemplate(0), indexTemplate(0)
{
    //initialize the static gl buffer templates
    generateVertexAttributeTemplate();
    generateIndexTemplate();

    //initialize the active front to be just the root node
    activeList.push_back(ActiveNode(iRootIndex, iRootBuffer, baseScope, false));
    QuadTerrain::computeChildScopes(activeList.begin());
    
    //initialize the shader
    shader.compileVertexShader("elevation.vs");
    shader.compileFragmentShader("elevation.fs");
    shader.linkShader();
    shader.useProgram();
    GLint geometryUniform = shader.getUniformLocation("geometry");
    glUniform1i(geometryUniform, 0);
    GLint heightUniform = shader.getUniformLocation("height");
    glUniform1i(heightUniform, 1);
    shader.disablePrograms();
}

QuadTerrain::GlData::
~GlData()
{
    if (vertexAttributeTemplate != 0)
        glDeleteBuffers(1, &vertexAttributeTemplate);
    if (indexTemplate != 0)
        glDeleteBuffers(1, &indexTemplate);
}


END_CRUSTA
