#include <crusta/QuadTerrain.h>

#include <algorithm>
#include <assert.h>

#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/DisplayState.h>
#include <Vrui/ViewSpecification.h>
#include <Vrui/Vrui.h>
#include <Vrui/VRWindow.h>

#include <crusta/CacheRequest.h>
#include <crusta/Crusta.h>
#include <crusta/DataManager.h>
#include <crusta/QuadCache.h>

///\todo remove debug
#include <GL/GLModels.h>

BEGIN_CRUSTA

static const uint NUM_GEOMETRY_INDICES =
    (TILE_RESOLUTION-1)*(TILE_RESOLUTION*2 + 2) - 2;
static const float TEXTURE_COORD_STEP  = 1.0 / TILE_RESOLUTION;
static const float TEXTURE_COORD_START = TEXTURE_COORD_STEP * 0.5;
static const float TEXTURE_COORD_END   = 1.0 - TEXTURE_COORD_START;

bool QuadTerrain::displayDebuggingBoundingSpheres = false;
bool QuadTerrain::displayDebuggingGrid            = false;

QuadTerrain::
QuadTerrain(uint8 patch, const Scope& scope, Crusta* iCrusta) :
    CrustaComponent(iCrusta), rootIndex(patch)
{
    crusta->getDataManager()->loadRoot(TreeIndex(patch), scope);
}


static GLFrustum<Scalar>
getFrustumFromVrui(GLContextData& contextData)
{
    const Vrui::DisplayState& displayState = Vrui::getDisplayState(contextData);
    Vrui::ViewSpecification viewSpec =
        displayState.window->calcViewSpec(displayState.eyeIndex);
    Vrui::NavTransform inv = Vrui::getInverseNavigationTransformation();

    GLFrustum<Scalar> frustum;
#if 1
    for (int i=0; i<8; ++i)
        frustum.setFrustumVertex(i,inv.transform(viewSpec.getFrustumVertex(i)));

	/* Calculate the six frustum face planes: */
	Vector3 fv10 = frustum.getFrustumVertex(1) - frustum.getFrustumVertex(0);
	Vector3 fv20 = frustum.getFrustumVertex(2) - frustum.getFrustumVertex(0);
	Vector3 fv40 = frustum.getFrustumVertex(4) - frustum.getFrustumVertex(0);
	Vector3 fv67 = frustum.getFrustumVertex(6) - frustum.getFrustumVertex(7);
	Vector3 fv57 = frustum.getFrustumVertex(5) - frustum.getFrustumVertex(7);
	Vector3 fv37 = frustum.getFrustumVertex(3) - frustum.getFrustumVertex(7);

    Vrui::Plane planes[8];
	planes[0] = Vrui::Plane(Geometry::cross(fv40,fv20),
                            frustum.getFrustumVertex(0));
	planes[1] = Vrui::Plane(Geometry::cross(fv57,fv37),
                            frustum.getFrustumVertex(7));
	planes[2] = Vrui::Plane(Geometry::cross(fv10,fv40),
                            frustum.getFrustumVertex(0));
	planes[3] = Vrui::Plane(Geometry::cross(fv37,fv67),
                            frustum.getFrustumVertex(7));
	planes[4] = Vrui::Plane(Geometry::cross(fv20,fv10),
                            frustum.getFrustumVertex(0));
	planes[5] = Vrui::Plane(Geometry::cross(fv67,fv57),
                            frustum.getFrustumVertex(7));
                    
    Scalar screenArea = Geometry::mag(planes[4].getNormal());
	for(int i=0; i<6; ++i)
		planes[i].normalize();
	
    for (int i=0; i<6; ++i)
        frustum.setFrustumPlane(i, planes[i]);

	/* Use the frustum near plane as the screen plane: */
    frustum.setScreenEye(planes[4], inv.transform(viewSpec.getEye()));
	
	/* Get viewport size from OpenGL: */
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT,viewport);
	
	/* Calculate the inverse pixel size: */
	frustum.setPixelSize(Math::sqrt((Scalar(viewport[2])*Scalar(viewport[3]))/screenArea));
#else
    for (int i=0; i<8; ++i)
        frustum.setFrustumVertex(i,inv.transform(viewSpec.getFrustumVertex(i)));
    for (int i=0; i<6; ++i)
    {
        Vrui::Plane plane = viewSpec.getFrustumPlane(i);
        frustum.setFrustumPlane(i, plane.transform(inv));
    }

    Vrui::Plane screenPlane = viewSpec.getScreenPlane();
    frustum.setScreenEye(screenPlane.transform(inv),
                         inv.transform(viewSpec.getEye()));


    Vector3 fv10 = frustum.getFrustumVertex(1) - frustum.getFrustumVertex(0);
    Vector3 fv20 = frustum.getFrustumVertex(2) - frustum.getFrustumVertex(0);

    const int* vSize  = viewSpec.getViewportSize();
    Scalar screenArea = Geometry::mag(Geometry::cross(fv20,fv10));
    Scalar pixelSize  = Math::sqrt((vSize[0]*vSize[1]) / screenArea);

    frustum.setPixelSize(pixelSize);
#endif

    return frustum;
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

    glData->shader.enable();
    glData->shader.setTextureStep(TEXTURE_COORD_STEP);
    glData->shader.setVerticalScale(crusta->getVerticalScale());
//    glUniform1f(glData->verticalScaleUniform, crusta->getVerticalScale());

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);

    //setup the evaluators
    FrustumVisibility visibility;
    visibility.frustum = getFrustumFromVrui(contextData);
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
        crusta->getCache()->getMainCache().findCached(rootIndex);
    assert(rootBuf != NULL);

    draw(contextData, glData, visibility, lod, rootBuf, actives, dataRequests);

    //restore the GL transform as it was before
    glPopMatrix();

    glData->shader.disable();

    glPopClientAttrib();
    glPopAttrib();

    glActiveTexture(activeTexture);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);

    //merge the data requests and active sets
    crusta->getCache()->getMainCache().request(dataRequests);
    crusta->submitActives(actives);
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
        videoBuf->touch(crusta);
        return videoBuf->getData();
    }
    else
    {
        //in any case the data has to be transfered from main memory
        if (videoBuf)
            videoBuf->touch(crusta);
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

    glData->shader.setCentroid(mainData.centroid[0], mainData.centroid[1],
                               mainData.centroid[2]);

//    glPolygonMode(GL_FRONT, GL_LINE);

    static const float ambient[4]  = {0.4, 0.4, 0.4, 1.0};
    static const float diffuse[4]  = {1.0, 1.0, 1.0, 1.0};
    static const float specular[4] = {0.3, 0.3, 0.3, 1.0};
    static const float emission[4] = {0.0, 0.0, 0.0, 1.0};
    static const float shininess   = 55.0;
    glMaterialfv(GL_FRONT, GL_AMBIENT,   ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  specular);
    glMaterialfv(GL_FRONT, GL_EMISSION,  emission);
    glMaterialf (GL_FRONT, GL_SHININESS, shininess);
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

if (displayDebuggingBoundingSpheres)
{
glData->shader.disable();
    GLint activeTexture;
    glPushAttrib(GL_ENABLE_BIT || GL_POLYGON_BIT);
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);

    glDisable(GL_LIGHTING);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT, GL_LINE);
    glPushMatrix();
    glColor3f(0.5f,0.5f,0.5f);
    glTranslatef(mainData.boundingCenter[0], mainData.boundingCenter[1],
                 mainData.boundingCenter[2]);
    glDrawSphereIcosahedron(mainData.boundingRadius, 2);

    glPopMatrix();
    glPopAttrib();
    glActiveTexture(activeTexture);
glData->shader.enable();
}

if (displayDebuggingGrid)
{
glData->shader.disable();
    GLint activeTexture;
    glPushAttrib(GL_ENABLE_BIT);
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    Point3* c = mainData.scope.corners;
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

    glPopAttrib();
    glActiveTexture(activeTexture);
glData->shader.enable();
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
                    children[i] = crusta->getCache()->getMainCache().findCached(
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
                    else if (!children[i]->isCurrent(crusta))
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
    vertexAttributeTemplate(0), indexTemplate(0)
{
    //initialize the static gl buffer templates
    generateVertexAttributeTemplate();
    generateIndexTemplate();
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
    GlData* glData = new GlData(crusta->getCache()->getVideoCache(contextData));
    //commit the context data
    contextData.addDataItem(this, glData);
}


END_CRUSTA
