#include <crusta/QuadTerrain.h>

#include <algorithm>
#include <assert.h>

#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <crusta/CacheRequest.h>
#include <crusta/Crusta.h>
#include <crusta/DataManager.h>
#include <crusta/QuadCache.h>

BEGIN_CRUSTA

static const uint NUM_GEOMETRY_INDICES =
    (TILE_RESOLUTION-1)*(TILE_RESOLUTION*2 + 2) - 2;
static const float TEXTURE_COORD_STEP  = 1.0 / TILE_RESOLUTION;
static const float TEXTURE_COORD_START = TEXTURE_COORD_STEP * 0.5;
static const float TEXTURE_COORD_END   = 1.0 - TEXTURE_COORD_START;

bool QuadTerrain::displayDebuggingGrid = false;

QuadTerrain::
QuadTerrain(uint8 patch, const Scope& scope) :
    rootIndex(patch)
{
    Crusta::getDataManager()->loadRoot(TreeIndex(patch), scope);
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

    glData->shader.useProgram();

    glUniform1f(glData->verticalScaleUniform, Crusta::getVerticalScale());

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);

    //setup the evaluators
    FrustumVisibility visibility;
    visibility.frustum.setFromGL();
    FocusViewEvaluator lod;
    lod.frustum = visibility.frustum;
    lod.setFocusFromDisplay();

    /* display could be multi-threaded. Buffer all the node data requests and
       merge them into the request list en block */
    CacheRequests dataRequests;
    /* as for requests for new data, buffer all the active node sets and submit
       them at the end */
    std::vector<MainCacheBuffer*> actives;

    /* save the current openGL transform. We are going to replace it during
       traversal with a scope centroid centered one to alleviate floating point
       issues with rotating vertices far off the origin */
    glPushMatrix();

    /* traverse the terrain tree, update as necessary and issue drawing commands
       for active nodes */
    MainCacheBuffer* rootBuf = 
        crustaQuadCache.getMainCache().findCached(rootIndex);
    assert(rootBuf != NULL);

    draw(contextData, glData, visibility, lod, rootBuf, actives, dataRequests);

    //restore the GL transform as it was before
    glPopMatrix();

    glData->shader.disablePrograms();

    glPopClientAttrib();
    glPopAttrib();

    glActiveTexture(activeTexture);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);

    //merge the data requests and active sets
    crustaQuadCache.getMainCache().request(dataRequests);
    Crusta::submitActives(actives);
}

const QuadNodeVideoData& QuadTerrain::
prepareGlData(GlData* glData, QuadNodeMainData& mainData)
{
    bool existed;
    VideoCacheBuffer* videoBuf = glData->videoCache.getBuffer(mainData.index,
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
}

void QuadTerrain::
drawNode(GLContextData& contextData, GlData* glData, QuadNodeMainData& mainData)
{
///\todo accommodate for lazy data fetching
    const QuadNodeVideoData& data = prepareGlData(glData, mainData);

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
        mainData.centroid[0], mainData.centroid[1], mainData.centroid[2]);
    Vrui::NavTransform nav =
    Vrui::getDisplayState(contextData).modelviewNavigational;
    nav *= Vrui::NavTransform::translate(centroidTranslation);
    glLoadMatrix(nav);

    glUniform3f(glData->centroidUniform, mainData.centroid[0],
                mainData.centroid[1], mainData.centroid[2]);

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
    Scope::Vertex* c = mainData.scope.corners;
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
    Scope::Vertex* c = mainData.scope.corners;
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
draw(GLContextData& contextData, GlData* glData,
     FrustumVisibility& visibility, FocusViewEvaluator& lod,
     MainCacheBuffer* node, std::vector<MainCacheBuffer*>& actives,
     CacheRequests& requests)
{
    //confirm current node as being active
    actives.push_back(node);
    QuadNodeMainData& mainData = node->getData();

    float visible = visibility.evaluate(mainData);
    if (visible)
    {
        //evaluate node for splitting
        float lodValue = lod.evaluate(mainData);
        if (lodValue>1.0)
        {
            bool allgood = false;
            //check if any of the children actually have data
            for (int i=0; i<4; ++i)
            {
                if (mainData.childDemTiles[i]  !=  DemFile::INVALID_TILEINDEX ||
                    mainData.childColorTiles[i]!=ColorFile::INVALID_TILEINDEX)
                {
                    allgood = true;
                }
            }
            //check if all the children are cached
            MainCacheBuffer* children[4];
            if (allgood)
            {
                for (int i=0; i<4; ++i)
                {
                    children[i] = crustaQuadCache.getMainCache().findCached(
                        mainData.index.down(i));
                    if (children[i] == NULL)
                    {
                        //request the data be loaded
                        requests.push_back(CacheRequest(lodValue, node, i));
                        allgood = false;
                    }
                }
            }
            //check that all the children are current
            if (allgood)
            {
                for (int i=0; i<4; ++i)
                {
                    if (!children[i]->isValid())
                        allgood = false;
                    else if (!children[i]->isCurrent())
                    {
                        //"request" it for update
                        actives.push_back(children[i]);
                        allgood = false;
                    }
                }
            }
            //still all good then recurse to the children
            if (allgood)
            {
                for (int i=0; i<4; ++i)
                {
                    draw(contextData, glData, visibility, lod, children[i],
                         actives, requests);
                }
            }
            else
                drawNode(contextData, glData, mainData);
        }
        else
            drawNode(contextData, glData, mainData);
    }
}




QuadTerrain::GlData::
GlData(VideoCache& iVideoCache) :
    videoCache(iVideoCache),
    vertexAttributeTemplate(0), indexTemplate(0), verticalScaleUniform(0),
    centroidUniform(0)
{
    //initialize the static gl buffer templates
    generateVertexAttributeTemplate();
    generateIndexTemplate();

    //initialize the shader
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
    //allocate the context dependent data
    GlData* glData = new GlData(crustaQuadCache.getVideoCache(contextData));
    //commit the context data
    contextData.addDataItem(this, glData);
}


END_CRUSTA
