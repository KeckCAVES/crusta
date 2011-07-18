#include <crusta/QuadTerrain.h>

#include <algorithm>
#include <assert.h>

#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLMaterialTemplates.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/DisplayState.h>
#include <Vrui/ViewSpecification.h>
#include <Vrui/Vrui.h>
#include <Vrui/VRWindow.h>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>
#include <crusta/DataManager.h>
#include <crusta/Homography.h>
#include <crusta/LightingShader.h>
#include <crusta/map/MapManager.h>
#include <crusta/map/Polyline.h>
#include <crusta/QuadCache.h>
#include <crusta/Triangle.h>
#include <crusta/Section.h>
#include <crusta/Sphere.h>

#define DO_RELATIVE_LEAF_TRIANGLE_INTERSECTIONS 1

#if DEBUG_INTERSECT_CRAP
#define DEBUG_INTERSECT_SIDES 0
#define DEBUG_INTERSECT_PEEK 0
#endif //DEBUG_INTERSECT_CRAP
#include <crusta/CrustaVisualizer.h>
#define CV(x) CrustaVisualizer::x

///\todo used for debugspheres
#include <GL/GLModels.h>

///\todo debug remove
#include <Geometry/ProjectiveTransformation.h>


BEGIN_CRUSTA


static const uint NUM_GEOMETRY_INDICES =
    (TILE_RESOLUTION-1)*(TILE_RESOLUTION*2 + 2) - 2;
static const float TEXTURE_COORD_START = TILE_TEXTURE_COORD_STEP * 0.5;
static const float TEXTURE_COORD_END   = 1.0 - TEXTURE_COORD_START;

bool QuadTerrain::displayDebuggingBoundingSpheres = false;
bool QuadTerrain::displayDebuggingGrid            = false;

///\todo dependency on VruiGlew must be dynamically allocated after VruiGlew
QuadTerrain::GlData* QuadTerrain::glData = 0;



static int
computeContainingChild(const Point3& pos, const Scope& scope)
{
    Section horizontal(Geometry::mid(scope.corners[2], scope.corners[3]),
                       Geometry::mid(scope.corners[0], scope.corners[1]));
    int leftRight = horizontal.isContained(pos) ? 1 : 0;

    Section vertical(Geometry::mid(scope.corners[0], scope.corners[2]),
                     Geometry::mid(scope.corners[1], scope.corners[3]));
    int downUp = vertical.isContained(pos) ? 2 : 0;

    return leftRight | downUp;
}

static void
computeExit(const Ray& ray, const double oldParam, const Scope& scope,
            double& param, int& side)
{
    const Point3* edgeCorners[4][2] = {
        {&scope.corners[3], &scope.corners[2]},
        {&scope.corners[2], &scope.corners[0]},
        {&scope.corners[0], &scope.corners[1]},
        {&scope.corners[1], &scope.corners[3]} };

    param = Math::Constants<double>::max;
    for (int i=0; i<4; ++i)
    {
        Section section(*(edgeCorners[i][0]), *(edgeCorners[i][1]));
CRUSTA_DEBUG(96,
CrustaVisualizer::addSection(section, 5);
CrustaVisualizer::show("Exit Side Search");)
        HitResult hit   = section.intersectPlane(ray);
        double hitParam = hit.getParameter();
        if (hit.isValid() && hitParam>=oldParam && hitParam<=param)
        {
            param = hitParam;
            side  = i;
CRUSTA_DEBUG(96,
CrustaVisualizer::addHit(ray, hit, 7);
CrustaVisualizer::show("Hit Side");
CrustaVisualizer::clear(7);
CrustaVisualizer::peek();)
        }
    }
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT <<
"Scope exit param: " << param << " side: " << side << "\n";)
}

QuadTerrain::
QuadTerrain(uint8 patch, const Scope& scope, Crusta* iCrusta) :
    CrustaComponent(iCrusta), rootIndex(patch)
{
    DATAMANAGER->loadRoot(crusta, rootIndex, scope);
}


const QuadTerrain::MainBuffer QuadTerrain::
getRootBuffer() const
{
    MainBuffer rootBuf;
    DATAMANAGER->find(rootIndex, rootBuf);
    assert(DATAMANAGER->isComplete(rootBuf));
    return rootBuf;
}

const QuadTerrain::MainData QuadTerrain::
getRootNode() const
{
    return DATAMANAGER->getData(getRootBuffer());
}

SurfacePoint QuadTerrain::
intersect(const Ray& ray, Scalar tin, int sin, Scalar& tout, int& sout,
          const Scalar gout) const
{
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT << "\n\nIntersecting Ray with Globe:\n\n";)
    return intersectNode(getRootBuffer(), ray, tin, sin, tout, sout, gout);
}


void QuadTerrain::
segmentCoverage(const Point3& start, const Point3& end,
                Shape::IntersectionFunctor& callback) const
{
    segmentCoverage(getRootBuffer(), start, end, callback);
}


void QuadTerrain::
renderLineCoverageMap(GLContextData& contextData, const MainData& nodeData)
{
    typedef Homography::HVector HVector;

    NodeData& node = *nodeData.node;

    //compute projection matrix
    Homography toNormalized;

    //destinations are fll, flr, ful, bll, bur
    toNormalized.setDestination(HVector(-1,-1,-1,1), HVector(1,-1,-1,1),
        HVector(-1,1,-1,1), HVector(-1,-1,1,1), HVector(1,1,1,1));

    //the elevation range might be flat. Make sure to give the frustum depth
    DemHeight::Type elevationRange[2];
    node.getElevationRange(elevationRange);
    Scalar sideLen = Geometry::dist(node.scope.corners[0],
                                    node.scope.corners[1]);
    if (Math::abs(elevationRange[0]-elevationRange[1]) < sideLen)
    {
        DemHeight::Type midElevation = (elevationRange[0] + elevationRange[1]) *
                                       DemHeight::Type(0.5);
        sideLen *= 0.5;
        elevationRange[0] = midElevation - sideLen;
        elevationRange[1] = midElevation + sideLen;
    }

    Point3 srcs[5];
    srcs[0] = node.scope.corners[0];
    srcs[1] = node.scope.corners[1];
    srcs[2] = node.scope.corners[2];
    srcs[3] = node.scope.corners[0];
    srcs[4] = node.scope.corners[3];

    //map the source points to planes
    Vector3 normal(node.centroid);
    normal.normalize();

    Geometry::Plane<Scalar,3> plane;
    plane.setNormal(-normal);
    plane.setPoint(Point3(normal*(SETTINGS->globeRadius +
                                  elevationRange[0])));
    for (int i=0; i<3; ++i)
    {
        Ray ray(Point3(0), srcs[i]);
        HitResult hit = plane.intersectRay(ray);
        assert(hit.isValid());
        srcs[i]    = ray(hit.getParameter());
    }
    plane.setPoint(Point3(normal*(SETTINGS->globeRadius +
                                  elevationRange[1])));
    for (int i=3; i<5; ++i)
    {
        Ray ray(Point3(0), srcs[i]);
        HitResult hit = plane.intersectRay(ray);
        assert(hit.isValid());
        srcs[i] = ray(hit.getParameter());
    }

    //make the source points relative to the node's centroid
    for (int i=0; i<5; ++i)
    {
        for (int j=0; j<3; ++j)
            srcs[i][j] -= node.centroid[j];
    }
    toNormalized.setSource(HVector(srcs[0][0], srcs[0][1], srcs[0][2], 1),
                           HVector(srcs[1][0], srcs[1][1], srcs[1][2], 1),
                           HVector(srcs[2][0], srcs[2][1], srcs[2][2], 1),
                           HVector(srcs[3][0], srcs[3][1], srcs[3][2], 1),
                           HVector(srcs[4][0], srcs[4][1], srcs[4][2], 1));

    toNormalized.computeProjective();

    //switch to the line coverage rendering shader
///\todo this is bad. Just to test GlewObject
    GlData::Item* glItem = contextData.retrieveDataItem<GlData::Item>(glData);
    glItem->lineCoverageShader.push();

    //convert the projection matrix to floating point and assign to the shader
    GLfloat projMat[16];
    for (int j=0; j<4; ++j)
    {
        for (int i=0; i<4; ++i)
            projMat[j*4+i] = toNormalized.getProjective().getMatrix()(i,j);
    }
    glUniformMatrix4fv(glItem->lineCoverageTransformUniform, 1, false, projMat);

    //setup openGL for rendering the texture
    GLdouble depthRange[2];
    glGetDoublev(GL_DEPTH_RANGE, depthRange);
    glDepthRange(0.0, 0.0);

    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

    //clear the old coverage map
    glEnable(GL_SCISSOR_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);

#if 0
    Point3 geo0(nodeData.geometry[0].position[0],
                nodeData.geometry[0].position[1],
                nodeData.geometry[0].position[2]);
    Point3 geo1(nodeData.geometry[1].position[0],
                nodeData.geometry[1].position[1],
                nodeData.geometry[1].position[2]);
    double cellSize = Geometry::dist(geo0, geo1);
    cellSize       *= TILE_RESOLUTION / SETTINGS->lineCoverageTexSize;
///\todo have to find the scaleFac that corresponds to the coarsening transition of the node
    float scaleFac  = Vrui::getNavigationTransformation().getScaling();
    float lineWidth = SETTINGS->lineSymbolWidth / scaleFac;
    lineWidth       = Math::ceil(lineWidth/cellSize);
#else
    float lineWidth       = 15.0f;
#endif

    glLineWidth(lineWidth);

    glBegin(GL_LINES);

    typedef std::vector<int> Ints;

    const Colors& data    = node.lineData;
    const Ints&   offsets = node.lineCoverageOffsets;

    for (Ints::const_iterator oit=offsets.begin(); oit!=offsets.end(); ++oit)
    {
        const int offset = *oit;

        //pass the offset as an appropriate color
        const float c[2]  = {              (offset&0xFF) / 255.0f,
                             (((offset>>8) & 0xFF) + 64) / 255.0f };
        glColor4f(c[0], c[1], c[0], c[1]);

        //grab the segment end points from the line data
        const Color& start = data[offset+1];
        const Color&   end = data[offset+2];

        //draw the
        glVertex3f(start[0], start[1], start[2]);
        glVertex3f(  end[0],   end[1],   end[2]);
    }

    glEnd();

    //clean up all the state changes
    glPopAttrib();
    glDepthRange(depthRange[0], depthRange[1]);
    glItem->lineCoverageShader.pop();
}


static GLFrustum<Scalar>
getFrustumFromVrui(GLContextData& contextData)
{
    const Vrui::DisplayState& displayState = Vrui::getDisplayState(contextData);
    Vrui::ViewSpecification viewSpec =
        displayState.window->calcViewSpec(displayState.eyeIndex);
    Vrui::NavTransform inv = Vrui::getInverseNavigationTransformation();

    GLFrustum<Scalar> frustum;

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

    return frustum;
}


void QuadTerrain::
prepareDisplay(GLContextData& contextData, SurfaceApproximation& surface)
{
    //setup the evaluators
    FrustumVisibility visibility;
    visibility.frustum = getFrustumFromVrui(contextData);
    FocusViewEvaluator lod;
    lod.frustum = visibility.frustum;
    lod.setFocusFromDisplay();

    /* display could be multi-threaded. Buffer all the node data requests and
       merge them into the request list en block */
    DataManager::Requests dataRequests;

    /* traverse the terrain tree, update as necessary and collect the current
       tree front */
    MainBuffer rootBuf = getRootBuffer();
    prepareDisplay(visibility, lod, rootBuf, surface, dataRequests);

    //merge the data requests
    DATAMANAGER->request(dataRequests);
}

void QuadTerrain::
display(GLContextData& contextData, CrustaGlData* crustaGl,
        SurfaceApproximation& surface)
{
    GpuCache& gpuCache = CACHE->getGpuCache(contextData);

//- setup the GL
    GLint arrayBuffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
    GLint elementArrayBuffer;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementArrayBuffer);

    glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_LINE_BIT |
                 GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);
    CHECK_GLA

    //setup transparent rendering
    if (SETTINGS->terrainDiffuseColor[3] < 0.95f)
    {
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glEnable(GL_CULL_FACE);
    }

    //setup the texturing
    glActiveTexture(GL_TEXTURE0);
    gpuCache.geometry.bind();
    glActiveTexture(GL_TEXTURE1);
    gpuCache.color.bind();
    glActiveTexture(GL_TEXTURE2);
    gpuCache.layerf.bind();
    CHECK_GLA

    if (SETTINGS->lineDecorated)
    {
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, crustaGl->symbolTex);
        glActiveTexture(GL_TEXTURE3);
        gpuCache.lineData.bind();
        glActiveTexture(GL_TEXTURE4);
        gpuCache.coverage.bind();
        CHECK_GLA;
    }

    glMaterialAmbient(GLMaterialEnums::FRONT,
                      SETTINGS->terrainAmbientColor);
    glMaterialDiffuse(GLMaterialEnums::FRONT,
                      SETTINGS->terrainDiffuseColor);
    glMaterialEmission(GLMaterialEnums::FRONT,
                       SETTINGS->terrainEmissiveColor);
    glMaterialSpecular(GLMaterialEnums::FRONT,
                       SETTINGS->terrainSpecularColor);
    glMaterialShininess(GLMaterialEnums::FRONT,
                        SETTINGS->terrainShininess);
    CHECK_GLA;

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);
    CHECK_GLA;

    /* save the current openGL transform. We are going to replace it during
       traversal with a scope centroid centered one to alleviate floating point
       issues with rotating vertices far off the origin */
    glPushMatrix();

    //render the terrain nodes in batches
    DataManager::Batch batch;
    DATAMANAGER->startGpuBatch(contextData, surface, batch);
    while (!batch.empty())
    {
        //draw the nodes of the current batch
        for (DataManager::Batch::const_iterator it=batch.begin();
             it!=batch.end(); ++it)
        {
            drawNode(contextData, crustaGl, it->main, it->gpu);
        }

        //grab the next batch
        DATAMANAGER->nextGpuBatch(contextData, surface, batch);
    }

    //restore the GL transform as it was before
    glPopMatrix();

    glPopClientAttrib();
    glPopAttrib();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
}


void QuadTerrain::
initGlData()
{
    assert(glData == NULL);
    glData = new GlData;
}

void QuadTerrain::
deleteGlData()
{
    assert(glData != NULL);
    delete glData;
}

QuadTerrain::GlData::Item::
Item()
{
    //create the mesh attributes
    generateVertexAttributeTemplate(vertexAttributeTemplate);
    generateIndexTemplate(indexTemplate);

    //create the shader to process the line coverages into the corresponding map
    std::string vp;
    vp += "uniform mat4 transform;\n";
    vp += "void main()\n{\n";
    vp += "gl_Position = transform * gl_Vertex;\n";
    vp += "gl_Position /= gl_Position.w;\n";
    vp += "gl_FrontColor = gl_Color;\n}\n";

    std::string fp = "void main()\n{\ngl_FragColor = gl_Color;\n}\n";

    lineCoverageShader.addString(GL_VERTEX_SHADER, vp);
    lineCoverageShader.addString(GL_FRAGMENT_SHADER, fp);
    lineCoverageShader.link();

    lineCoverageShader.push();
    lineCoverageTransformUniform = lineCoverageShader.getUniformLocation(
        "transform");
    lineCoverageShader.pop();
}

QuadTerrain::GlData::Item::
~Item()
{
    glDeleteBuffers(1, &vertexAttributeTemplate);
    glDeleteBuffers(1, &indexTemplate);
}


void QuadTerrain::GlData::
initContext(GLContextData& contextData) const
{
/** \todo fixme, GlProgram shouldn't require code here, but the contextData is
needed for the VruiGlew initialization and GlProgram assumes it is already "in
context" */
    VruiGlew::enable(contextData); //needed for the GlProgram

    Item* item = new Item;
    contextData.addDataItem(this, item);
}


void QuadTerrain::
generateVertexAttributeTemplate(GLuint& vertexAttributeTemplate)
{
    /** allocate some temporary main memory to store the vertex attributes
        before they are streamed to the GPU */
    uint numTexCoords = TILE_RESOLUTION*TILE_RESOLUTION*2;
    float* positionsInMemory = new float[numTexCoords];
    float* positions = positionsInMemory;

    /** generate a set of normalized texture coordinates */
    for (float y = TEXTURE_COORD_START;
         y < (TEXTURE_COORD_END + 0.1*TILE_TEXTURE_COORD_STEP);
         y += TILE_TEXTURE_COORD_STEP)
    {
        for (float x = TEXTURE_COORD_START;
             x < (TEXTURE_COORD_END + 0.1*TILE_TEXTURE_COORD_STEP);
             x += TILE_TEXTURE_COORD_STEP, positions+=2)
        {
            positions[0] = x;
            positions[1] = y;
        }
    }

    //generate the vertex buffer and stream in the data
    CHECK_GL_CLEAR_ERROR;

    GLint arrayBuffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);

    glGenBuffers(1, &vertexAttributeTemplate);
    glBindBuffer(GL_ARRAY_BUFFER, vertexAttributeTemplate);
    glBufferData(GL_ARRAY_BUFFER, numTexCoords*sizeof(float), positionsInMemory,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);

    CHECK_GL_THROW_ERROR;

    //clean-up
    delete[] positionsInMemory;
}

void QuadTerrain::
generateIndexTemplate(GLuint& indexTemplate)
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
    CHECK_GL_CLEAR_ERROR;

    GLint elementArrayBuffer;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementArrayBuffer);

    glGenBuffers(1, &indexTemplate);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexTemplate);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, NUM_GEOMETRY_INDICES*sizeof(uint16),
                 indicesInMemory, GL_STATIC_DRAW);

    CHECK_GL_THROW_ERROR;

    //clean-up
    delete[] indicesInMemory;
}


SurfacePoint QuadTerrain::
intersectNode(const MainBuffer& nodeBuf, const Ray& ray,
              double tin, int sin, double& tout, int& sout,
              const double gout) const
{
    const double& verticalScale = crusta->getVerticalScale();

    MainData mainData = DATAMANAGER->getData(nodeBuf);
    const NodeData& node = *mainData.node;

CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT <<
"++++++++++++++++++++++++ " << node.index.med_str() << "\n";)
CRUSTA_DEBUG(94, CrustaVisualizer::addScope(node.scope);)
CRUSTA_DEBUG(94, CrustaVisualizer::addScope(node.scope, 3, Color(1,0,0,1));)
CRUSTA_DEBUG(95,
Ray blarg(ray.getOrigin(), ray(300000000.0));
CrustaVisualizer::addRay(blarg,3);
CrustaVisualizer::addHit(ray, tin, 4);)

//- determine the exit point and side
    computeExit(ray, tin, node.scope, tout, sout);
    if (tout==Math::Constants<double>::max)
    {
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT << "------------------------\n";)
        return SurfacePoint();
    }

//- check intersection with upper boundary
    DemHeight::Type elevationRange[2];
    node.getElevationRange(elevationRange);

    Sphere shell(Point3(0), SETTINGS->globeRadius +
                 verticalScale*elevationRange[1]);
    double t0, t1;
    bool intersects = shell.intersectRay(ray, t0, t1);

CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT <<
"Shell: " << intersects << " " << t0 << " " << t1 << "\n";)

    /* does it intersect: not hit the shell at all OR after exited scope
       OR exited shell before current point */
    if (!intersects || t0>tout || t1<tin)
    {
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT << "------------------------\n";)
        return SurfacePoint();
    }

///\todo check lower boundary?

//- perform leaf intersection?
    if (!DATAMANAGER->existsChildData(mainData))
    {
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT <<
"No children exist, considering leaf.\n";)
        SurfacePoint sp = intersectLeaf(mainData, ray, tin, sin, gout);
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT << "------------------------\n";)
        return sp;
    }

//- determine starting child
    int childId = computeContainingChild(ray(tin), node.scope);
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT <<
"Next child: " << childId << "\n";)

//- continue traversal
    TreeIndex childIndex = node.index.down(childId);
    MainBuffer childBuf;
    bool childExists = DATAMANAGER->find(childIndex, childBuf);

    double ctin  = tin;
    double ctout = 0.0;
    int    csin  = sin;
    int    csout = -1;

    while (true)
    {
        if (childExists && DATAMANAGER->isCurrent(childBuf))
        {
            //recurse
            SurfacePoint surfacePoint = intersectNode(childBuf, ray, ctin, csin,
                                                      ctout, csout, gout);
            if (surfacePoint.isValid())
            {
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT << "------------------------\n";)
                return surfacePoint;
            }
            else
            {
                ctin = ctout;
                if (ctin > gout)
                {
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT << "Reached global exit point\n";)
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT << "------------------------\n";)
                    return SurfacePoint();
                }

                //move to the next child
                static const int next[4][4][2] = {
                    { { 2, 2}, {-1,-1}, {-1,-1}, { 1, 1} },
                    { { 3, 2}, { 0, 3}, {-1,-1}, {-1,-1} },
                    { {-1,-1}, {-1,-1}, { 0, 0}, { 3, 1} },
                    { {-1,-1}, { 2, 3}, { 1, 0}, {-1,-1} } };
                csin    = next[childId][csout][1];
                childId = next[childId][csout][0];
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT <<
"Next child: " << childId << "\n";)
                if (childId == -1)
                {
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT << "------------------------\n";)
                    return SurfacePoint();
                }

                childIndex  = node.index.down(childId);
                childExists = DATAMANAGER->find(childIndex, childBuf);
            }
        }
        else
        {
///\todo Vis2010 simplify. Don't allow loads of nodes from here
//            mainCache.request(MainCache::Request(0.0, nodeBuf, childId));
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT <<
"Child does not exist, considering leaf.\n";)
            SurfacePoint sp = intersectLeaf(mainData, ray, tin, sin, gout);
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT << "------------------------\n";)
            return sp;
        }
    }

    //execution should never reach this point
    assert(false);
    return SurfacePoint();
}

SurfacePoint QuadTerrain::
intersectLeaf(const MainData& leafData, const Ray& ray,
              double param, int side, const double gout) const
{
    NodeData& leaf = *leafData.node;

    SurfacePoint surfacePoint;
    surfacePoint.nodeIndex = leaf.index;

CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT <<
"*** " << leaf.index.med_str() << "\n";)

//- locate the starting cell
CRUSTA_DEBUG(96, CrustaVisualizer::addHit(ray, param, 7);)

    int tileRes = TILE_RESOLUTION;
    int cellX   = 0;
    int cellY   = 0;

    int numLevels = 1;
    int res       = tileRes-1;
    while (res > 1)
    {
        ++numLevels;
        res >>= 1;
    }

    Scope scope   = leaf.scope;
    cellX         = 0;
    cellY         = 0;
    int shift     = (tileRes-1) >> 1;
    for (int level=1; level<numLevels; ++level)
    {
CRUSTA_DEBUG(96,
CrustaVisualizer::addScope(scope);
CrustaVisualizer::show("Cell Search");)
        //compute the child containing the current position
        int childIndex = computeContainingChild(ray(param), scope);

        //adjust the current bottom-left offset
        cellX += childIndex&0x1 ? shift : 0;
        cellY += childIndex&0x2 ? shift : 0;
        shift >>= 1;

        //switch to the child scope
        Scope childScopes[4];
        scope.split(childScopes);
        scope = childScopes[childIndex];
    }

CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT <<
"Cell: " << cellX << " " << cellY << "\n";)

//- traverse cells
#if DO_RELATIVE_LEAF_TRIANGLE_INTERSECTIONS
    Ray relativeRay(Point3(Vector3(ray.getOrigin())-Vector3(leaf.centroid)),
                    ray.getDirection());
#else
    Ray relativeRay(ray);
#endif //DO_RELATIVE_LEAF_TRIANGLE_INTERSECTIONS
CRUSTA_DEBUG(96,
Ray blarg(relativeRay.getOrigin(), relativeRay(300000000.0));
CrustaVisualizer::addRay(blarg, 1);
CrustaVisualizer::peek();)

    double verticalScale = crusta->getVerticalScale();
    int offset = cellY*tileRes + cellX;
    Vertex*          cellV = leafData.geometry + offset;
    DemHeight::Type* cellH = leafData.height   + offset;
    while (true)
    {
        const Vertex::Position* positions[4] = {
            &(cellV->position), &((cellV+1)->position),
            &((cellV+tileRes)->position), &((cellV+tileRes+1)->position) };
        DemHeight::Type heights[4] = {
            leaf.getHeight(*cellH),           leaf.getHeight(*(cellH+1)),
            leaf.getHeight(*(cellH+tileRes)), leaf.getHeight(*(cellH+tileRes+1))
        };

        //construct the corners of the current cell
        Vector3 cellCorners[4];
        Vector3 relativeCellCorners[4];
        for (int i=0; i<4; ++i)
        {
            for (int j=0; j<3; ++j)
            {
                cellCorners[i][j] = double((*(positions[i]))[j]) +
                                    double(leaf.centroid[j]);
            }
            Vector3 extrude(cellCorners[i]);
            extrude.normalize();
            extrude        *= double(heights[i]) * verticalScale;
            cellCorners[i] += extrude;

            relativeCellCorners[i] = cellCorners[i];
#if DO_RELATIVE_LEAF_TRIANGLE_INTERSECTIONS
            relativeCellCorners[i]-= Vector3(leaf.centroid);
#endif //DO_RELATIVE_LEAF_TRIANGLE_INTERSECTIONS
        }

        //intersect triangles of current cell
        Triangle t0(relativeCellCorners[0],
                    relativeCellCorners[3],
                    relativeCellCorners[2]);
        Triangle t1(relativeCellCorners[0],
                    relativeCellCorners[1],
                    relativeCellCorners[3]);

CRUSTA_DEBUG(96,
CrustaVisualizer::addTriangle(t0, -1, Color(0.9, 0.6, 0.7, 1.0));
CrustaVisualizer::addTriangle(t1, -1, Color(0.7, 0.6, 0.9, 1.0));
//CrustaVisualizer::peek();
CrustaVisualizer::show("Intersecting triangles");)

        HitResult hit = t0.intersectRay(relativeRay);
        if (hit.isValid())
        {
            surfacePoint.cellIndex = Point2i(cellX, cellY);
///\todo compute the proper cell position
            surfacePoint.cellPosition = Point2(0.0, 0.0);
            surfacePoint.position = ray(hit.getParameter());
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT << "HIT! t0 \n";)

            return surfacePoint;
        }
        hit = t1.intersectRay(relativeRay);
        if (hit.isValid())
        {
            surfacePoint.cellIndex = Point2i(cellX, cellY);
///\todo compute the proper cell position
            surfacePoint.cellPosition = Point2(0.0, 0.0);
            surfacePoint.position = ray(hit.getParameter());
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT << "HIT! t0 \n";)

            return surfacePoint;
        }

        //determine the exit point and side
        computeExit(ray, param,
                    Scope(Point3(cellCorners[0]), Point3(cellCorners[1]),
                          Point3(cellCorners[2]), Point3(cellCorners[3])),
                    param, side);

        //end traversal if we did not find an exit point from the current cell
        if (param == Math::Constants<double>::max)
        {
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT << "No side intersection!\n";)
            return SurfacePoint();
        }

        //move to the next cell
        if (param > gout)
            return SurfacePoint();

        static const int next[4][3] = { {0,1,2}, {-1,0,3}, {0,-1,0}, {1,0,1} };

        cellX += next[side][0];
        cellY += next[side][1];
        if (cellX<0 || cellX>tileRes-2 || cellY<0 || cellY>tileRes-2)
            return SurfacePoint();

        offset = cellY*tileRes + cellX;
        cellV  = leafData.geometry + offset;
        cellH  = leafData.height   + offset;

        side = next[side][2];
CRUSTA_DEBUG(90, CRUSTA_DEBUG_OUT <<
"Cell: " << cellX << " " << cellY << "side: " << side << "\n";)
    }

    assert(false);
    return SurfacePoint();
}


void QuadTerrain::
segmentCoverage(const MainBuffer& nodeBuf,
                const Point3& start, const Point3& end,
                Shape::IntersectionFunctor& callback) const
{
    MainData  nodeData = DATAMANAGER->getData(nodeBuf);
    NodeData& node     = *nodeData.node;

    //end traversal if the segment does not overlap the current node
    if (!node.scope.intersects(start, end))
        return;

    //recursion to the children should only happen if all children are present
    bool       allChildren = true;
    MainBuffer childBuf[4];
    for (int i=0; i<4; ++i)
        allChildren &= DATAMANAGER->find(node.index.down(i), childBuf[i]);

    if (allChildren)
    {
        //traverse this node as an interior node
        callback(node, false);
        //recurse through the children
        for (int i=0; i<4; ++i)
            segmentCoverage(childBuf[i], start, end, callback);
    }
    else
    {
        //traverse this node as a leaf node
        callback(node, true);
    }
}


void QuadTerrain::
drawNode(GLContextData& contextData, CrustaGlData* crustaGl,
         const MainData& mainData, const GpuData& gpuData)
{
    NodeData& main = *mainData.node;

    //enable the terrain rendering shader
    crustaGl->terrainShader.enable();

///\todo this is bad. Just to test GlewObject
    GlData::Item* glItem = contextData.retrieveDataItem<GlData::Item>(glData);
    glBindBuffer(GL_ARRAY_BUFFER,         glItem->vertexAttributeTemplate);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glItem->indexTemplate);

    glVertexPointer(2, GL_FLOAT, 0, 0);
    CHECK_GLA

    //load the centroid relative translated navigation transformation
    glPushMatrix();
    Vrui::Vector centroidTranslation(
        main.centroid[0], main.centroid[1], main.centroid[2]);
    Vrui::NavTransform nav =
    Vrui::getDisplayState(contextData).modelviewNavigational;
    nav *= Vrui::NavTransform::translate(centroidTranslation);
    glLoadMatrix(nav);

    DataManager::SourceShaders& dataSources =
        DATAMANAGER->getSourceShaders(contextData);

    dataSources.geometry.setSubRegion(*gpuData.geometry);
    CHECK_GLA
    dataSources.height.setSubRegion(*gpuData.height);
    CHECK_GLA
    dataSources.topography.setCentroid(main.centroid);
    CHECK_GLA

    int numColorLayers = static_cast<int>(gpuData.colors.size());
    assert(numColorLayers == static_cast<int>(dataSources.colors.size()));
    for (int i=0; i<numColorLayers; ++i)
        dataSources.colors[i].setSubRegion(*gpuData.colors[i]);

    int numFloatLayers = static_cast<int>(gpuData.layers.size());
    assert(numFloatLayers == static_cast<int>(dataSources.layers.size()));
    for (int i=0; i<numFloatLayers; ++i)
        dataSources.layers[i].setSubRegion(*gpuData.layers[i]);

    if (SETTINGS->lineDecorated)
    {
        //setup the shader for decorated line drawing
        ShaderDecoratedLineRenderer& decorated =
            crustaGl->terrainShader.getDecoratedLineRenderer();
        decorated.setNumSegments(main.lineNumSegments);
        if (main.lineNumSegments > 0)
        {
            dataSources.coverage.setSubRegion(*gpuData.coverage);
            dataSources.lineData.setSubRegion((SubRegion)(*gpuData.lineData));
        }
        CHECK_GLA
    }

//    glPolygonMode(GL_FRONT, GL_LINE);

    glDrawRangeElements(GL_TRIANGLE_STRIP, 0,
                        (TILE_RESOLUTION*TILE_RESOLUTION) - 1,
                        NUM_GEOMETRY_INDICES, GL_UNSIGNED_SHORT, 0);
    glPopMatrix();
    CHECK_GLA

if (displayDebuggingBoundingSpheres)
{
crustaGl->terrainShader.disable();
    GLint activeTexture;
    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);

    glDisable(GL_LIGHTING);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT, GL_LINE);
    glPushMatrix();
    glColor3f(0.5f,0.5f,0.5f);
    glTranslatef(main.boundingCenter[0], main.boundingCenter[1],
                 main.boundingCenter[2]);
    glDrawSphereIcosahedron(main.boundingRadius, 1);

    glPopMatrix();
    glPopAttrib();
    glActiveTexture(activeTexture);
crustaGl->terrainShader.enable();
    CHECK_GLA
}

if (displayDebuggingGrid)
{
crustaGl->terrainShader.disable();
    CHECK_GLA
    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    CHECK_GLA
    Point3* c = main.scope.corners;
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
    CHECK_GLA

    glPopAttrib();
    glActiveTexture(activeTexture);
    CHECK_GLA
crustaGl->terrainShader.enable();
    CHECK_GLA
}

}

void QuadTerrain::
prepareDisplay(FrustumVisibility& visibility, FocusViewEvaluator& lod,
               MainBuffer& buf, SurfaceApproximation& surface,
               DataManager::Requests& requests)
{
    MapManager* mapMan = crusta->getMapManager();

    //confirm current node as being active
    DATAMANAGER->touch(buf);

    NodeMainData data = DATAMANAGER->getData(buf);

///\todo generalize this to an API that makes sure the node is ready for eval
    //make sure we have proper bounding spheres
    if (data.node->boundingAge < crusta->getLastScaleStamp())
    {
        data.node->computeBoundingSphere(SETTINGS->globeRadius,
            crusta->getVerticalScale());
    }

//- evaluate
    float visible = visibility.evaluate(*data.node);
    if (visible)
    {
        //evaluate node for splitting
        float lodValue = lod.evaluate(*data.node);
        if (lodValue>1.0)
        {
            //does there exist child data for refinement
            bool allgood = DATAMANAGER->existsChildData(data);
            //check if all the children are available
            NodeMainBuffer children[4];
            bool validChildren[4] = {false, false, false, false};
            if (allgood)
            {
                for (int i=0; i<4; ++i)
                {
                    if (!DATAMANAGER->find(data.node->index.down(i),
                                           children[i]))
                    {
                        //request the data be loaded
                        requests.push_back(DataManager::Request(
                            crusta, lodValue, buf, i));
                        allgood = false;
                    }
                    else
                        validChildren[i] = true;
                }
            }

/**\todo horrible Vis2010 HACK: integrate this in the proper way? I.e. don't
stall here, but defer the update. */
if (allgood && data.node->lineInheritCoverage)
{
    for (int i=0; i<4; ++i)
    {
        NodeMainData child = DATAMANAGER->getData(children[i]);
CRUSTA_DEBUG(60, std::cerr << "***COVDOWN parent(" << data.node->index <<
")    " << "n(" << data.node->index << ")\n\n";)
        mapMan->inheritShapeCoverage(*data.node, *child.node);
        child.node->lineInheritCoverage = true;
    }

    data.node->lineInheritCoverage = false;
}

            //still all good then recurse to the children
            if (allgood)
            {
                for (int i=0; i<4; ++i)
                    prepareDisplay(visibility,lod,children[i],surface,requests);
            }
            else
            {
                //make sure to hold on to the children that are already cached
                for (int i=0; i<4; ++i)
                {
                    if (validChildren[i])
                        DATAMANAGER->touch(children[i]);
                }
                //add the current node to the current representation
                surface.add(data, true);
            }
        }
        else
            surface.add(data, true);
    }
    else
        surface.add(data, false);
}




void QuadTerrain::
confirmLineCoverageRemoval(const MainData& nodeData, Shape* shape,
                           Shape::ControlPointHandle cp)
{
    typedef NodeData::ShapeCoverage        Coverage;
    typedef Shape::ControlPointHandleList  HandleList;
    typedef Shape::ControlPointConstHandle Handle;

    NodeData& node = *nodeData.node;

    //validate current node's coverage
    Coverage::const_iterator lit = node.lineCoverage.find(shape);
    if (lit != node.lineCoverage.end())
    {
        //check all the control point handles
        const HandleList& handles = lit->second;
        if (!handles.empty())
        {
            HandleList::const_iterator hit;
            for (hit=handles.begin(); hit!=handles.end() && *hit!=cp; ++hit);
            assert(hit == handles.end());
        }
    }

//- recurse
    MainBuffer children[4];

    //does there exist child data for refinement
    bool allgood = DATAMANAGER->existsChildData(nodeData);
    //check cached
    if (allgood)
    {
        for (int i=0; i<4; ++i)
        {
            if (!DATAMANAGER->find(node.index.down(i), children[i]))
                    allgood = false;
        }
    }
    //if good to go still, then the children are part of the active repr.
    if (allgood)
    {
        for (int i=0; i<4; ++i)
            confirmLineCoverageRemoval(DATAMANAGER->getData(children[i]),
                                       shape, cp);
    }
}

void QuadTerrain::
validateLineCoverage(const MainData& nodeData)
{
    typedef NodeData::ShapeCoverage        Coverage;
    typedef Shape::ControlPointHandleList  HandleList;
    typedef Shape::ControlPointConstHandle Handle;

    NodeData& node = *nodeData.node;

    MapManager*               mapMan = crusta->getMapManager();
    MapManager::PolylinePtrs& lines  = mapMan->getPolylines();

    //validate current node's coverage
    for (Coverage::const_iterator lit=node.lineCoverage.begin();
         lit!=node.lineCoverage.end(); ++lit)
    {
        //check that this line exists
        MapManager::PolylinePtrs::iterator lfit = std::find(lines.begin(),
            lines.end(), lit->first);
        assert(lfit != lines.end());

        //grab the polyline's controlpoints
        const Shape::ControlPointList& cpl = (*lfit)->getControlPoints();

        //check all the control point handles
        const HandleList& handles = lit->second;
        assert(!handles.empty());

        for (HandleList::const_iterator hit=handles.begin(); hit!=handles.end();
             ++hit)
        {
            //check existance
            Handle cfit;
            for (cfit=cpl.begin(); cfit!=cpl.end() && cfit!=*hit; ++cfit);
            assert(cfit != cpl.end());

            //check overlap
            Handle end = *hit; ++end;
            assert(node.scope.intersects((*hit)->pos, end->pos));

            //check duplicates
            HandleList::const_iterator nhit = hit;
            for (++nhit; nhit!=handles.end(); ++nhit)
                assert(*hit != *nhit);
        }
    }

//- recurse
    MainBuffer children[4];

    //does there exist child data for refinement
    bool allgood = DATAMANAGER->existsChildData(nodeData);
    //check cached
    if (allgood)
    {
        for (int i=0; i<4; ++i)
        {
            if (!DATAMANAGER->find(node.index.down(i), children[i]))
                    allgood = false;
        }
    }
    //if good to go still, then the children are part of the active repr.
    if (allgood)
    {
        for (int i=0; i<4; ++i)
            validateLineCoverage(DATAMANAGER->getData(children[i]));
    }
}


END_CRUSTA
