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
#define COVERAGE_PROJECTION_IN_GL 0

BEGIN_CRUSTA

static const uint NUM_GEOMETRY_INDICES =
    (TILE_RESOLUTION-1)*(TILE_RESOLUTION*2 + 2) - 2;
static const float TEXTURE_COORD_START = TILE_TEXTURE_COORD_STEP * 0.5;
static const float TEXTURE_COORD_END   = 1.0 - TEXTURE_COORD_START;

bool QuadTerrain::displayDebuggingBoundingSpheres = false;
bool QuadTerrain::displayDebuggingGrid            = false;

///\todo dependency on VruiGlew must be dynamically allocated after VruiGlew
QuadTerrain::GlData* QuadTerrain::glData = 0;



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

HitResult QuadTerrain::
intersect(const Ray& ray, Scalar tin, int sin, Scalar& tout, int& sout,
          const Scalar gout) const
{
    return intersectNode(getRootBuffer(), ray, tin, sin, tout, sout, gout);
}


static int
computeContainingChild(const Point3& p, int sideIn, const Scope& scope)
{
    const Point3* corners[4][2] = {
        {&scope.corners[3], &scope.corners[2]},
        {&scope.corners[2], &scope.corners[0]},
        {&scope.corners[0], &scope.corners[1]},
        {&scope.corners[1], &scope.corners[3]}};

    const Vector3 vp(p[0], p[1], p[2]);
    int childId   = ~0;
    int leftRight = ~0;
    int upDown    = ~0;
    switch (sideIn)
    {
        case -1:
        {
            Point3 mids    = Geometry::mid(*(corners[2][0]), *(corners[2][1]));
            Point3 mide    = Geometry::mid(*(corners[0][0]), *(corners[0][1]));
            Vector3 normal = Geometry::cross(Vector3(mids[0],mids[1],mids[2]),
                                             Vector3(mide[0],mide[1],mide[2]));

            leftRight = vp*normal>Scalar(0) ? 0 : 1;

            mids   = Geometry::mid(*(corners[3][0]), *(corners[3][1]));
            mide   = Geometry::mid(*(corners[1][0]), *(corners[1][1]));
            normal = Geometry::cross(Vector3(mids[0],mids[1],mids[2]),
                                             Vector3(mide[0],mide[1],mide[2]));

            upDown = vp*normal>Scalar(0) ? 0 : 2;
            break;
        }

        case 0:
        case 2:
        {
            Point3 mids    = Geometry::mid(*(corners[2][0]), *(corners[2][1]));
            Point3 mide    = Geometry::mid(*(corners[0][0]), *(corners[0][1]));
            Vector3 normal = Geometry::cross(Vector3(mids[0],mids[1],mids[2]),
                                             Vector3(mide[0],mide[1],mide[2]));

            leftRight = vp*normal>Scalar(0) ? 0 : 1;

            upDown = sideIn==2 ? 0 : 2;
            break;
        }

        case 1:
        case 3:
        {
            leftRight = sideIn==1 ? 0 : 1;

            Point3 mids    = Geometry::mid(*(corners[3][0]), *(corners[3][1]));
            Point3 mide    = Geometry::mid(*(corners[1][0]), *(corners[1][1]));
            Vector3 normal = Geometry::cross(Vector3(mids[0],mids[1],mids[2]),
                                             Vector3(mide[0],mide[1],mide[2]));

            upDown = vp*normal>Scalar(0) ? 0 : 2;
            break;
        }

        default:
            assert(false);
    }

    childId = leftRight | upDown;
    return childId;
}


void QuadTerrain::
intersect(Shape::IntersectionFunctor& callback, Ray& ray, Scalar tin, int sin,
          Scalar& tout, int& sout) const
{
    intersectNode(callback, getRootBuffer(), ray, tin, sin, tout, sout);
}


void QuadTerrain::
intersectNodeSides(const Scope& scope, const Ray& ray,
                   Scalar& tin, int& sin, Scalar& tout, int& sout)
{
    Section sections[4] = { Section(scope.corners[3], scope.corners[2]),
                            Section(scope.corners[2], scope.corners[0]),
                            Section(scope.corners[0], scope.corners[1]),
                            Section(scope.corners[1], scope.corners[3]) };

    sin  =  sout = -1;
    tin  =  Math::Constants<Scalar>::max;
    tout = -Math::Constants<Scalar>::max;
    for (int i=0; i<4; ++i)
    {
        HitResult hit   = sections[i].intersectRay(ray);
        Scalar hitParam = hit.getParameter();
        if (hit.isValid())
        {
            if (hitParam < tin)
            {
                tin = hitParam;
                sin = i;
            }
            if (hitParam > tout)
            {
                tout = hitParam;
                sout = i;
            }
        }
    }
}


void QuadTerrain::
renderLineCoverageMap(GLContextData& contextData, const MainData& nodeData)
{
    typedef NodeData::ShapeCoverage        Coverage;
    typedef Shape::ControlPointHandleList  HandleList;
    typedef Shape::ControlPointConstHandle Handle;

    NodeData& node = *nodeData.node;

    //compute projection matrix
    Homography toNormalized;
    //destinations are fll, flr, ful, bll, bur
    toNormalized.setDestination(Point3(-1,-1,-1), Point3(1,-1,-1),
        Point3(-1,1,-1), Point3(-1,-1,1), Point3(1,1,1));

    //the elevation range might be flat. Make sure to give the frustum depth
    DemHeight elevationRange[2] = { node.elevationRange[0],
                                    node.elevationRange[1] };
    Scalar sideLen = Geometry::dist(node.scope.corners[0],
                                    node.scope.corners[1]);
    if (Math::abs(elevationRange[0]-elevationRange[1]) < sideLen)
    {
        DemHeight midElevation = (elevationRange[0] + elevationRange[1]) * 0.5;
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
        srcs[i] = ray(hit.getParameter());
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

    toNormalized.setSource(srcs[0], srcs[1], srcs[2], srcs[3], srcs[4]);

    toNormalized.computeProjective();

    //switch to the line coverage rendering shader
///\todo this is bad. Just to test GlewObject
    GlData::Item* glItem = contextData.retrieveDataItem<GlData::Item>(glData);
    glItem->lineCoverageShader.push();
/**\todo it seems there might be an issue in converting the matrix to float or
simply float processing the transformation */
#if COVERAGE_PROJECTION_IN_GL
    //convert the projection matrix to floating point and assign to the shader
    GLfloat projMat[16];
    for (int j=0; j<4; ++j)
    {
        for (int i=0; i<4; ++i)
        {
#if 0
            projMat[j*4+i] = i==j ? 1.0 : 0.0;
#else
            projMat[j*4+i] = toNormalized.getProjective().getMatrix()(i,j);
#endif
        }
    }
    glUniformMatrix4fv(glItem->lineCoverageTransformUniform, 1, true, projMat);
#endif //COVERAGE_PROJECTION_IN_GL

    //setup openGL for rendering the texture
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

    //clear the old coverage map
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);

    const Coverage&  coverage = node.lineCoverage;
    const Vector2fs& offsets  = node.lineCoverageOffsets;

    glLineWidth(15.0f);

    glBegin(GL_LINES);

    //as the coverage is traversed also traverse the offsets
    Vector2fs::const_iterator oit = offsets.begin();
    //traverse all the line in the coverage
    for (Coverage::const_iterator lit=coverage.begin(); lit!=coverage.end();
         ++lit)
    {
#if DEBUG
        const Polyline* line = dynamic_cast<const Polyline*>(lit->first);
        assert(line != NULL);
#endif //DEBUG
        const HandleList& handles = lit->second;

        for (HandleList::const_iterator hit=handles.begin(); hit!=handles.end();
             ++hit, ++oit)
        {
            //pass the offset along
            Color color((*oit)[0], (*oit)[1], (*oit)[0], (*oit)[1]);
            glColor(color);

            Handle cur  = *hit;
            Handle next = cur; ++next;

#if 1
            //manually transform the points before passing to the GL
            typedef Geometry::HVector<double,3> HPoint;

            const Homography::Projective& p = toNormalized.getProjective();

            Point3 curPos  = p.transform(HPoint(cur->pos)).toPoint();
            Point3 nextPos = p.transform(HPoint(next->pos)).toPoint();

            Point3f curPosf (curPos[0],  curPos[1],  0.0f);
            Point3f nextPosf(nextPos[0], nextPos[1], 0.0f);

            glVertex3fv(curPosf.getComponents());
            glVertex3fv(nextPosf.getComponents());
#else
            //let the GL transform the points
            glVertex3dv(cur->pos.getComponents());
            glVertex3dv(next->pos.getComponents());
#endif
        }
    }

    glEnd();

    //clean up all the state changes
    glPopAttrib();
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
                 GL_POLYGON_BIT);
    CHECK_GLA

    glEnable(GL_CULL_FACE);

    //setup the texturing
    glEnable(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0);
    gpuCache.geometry.bind();
    glActiveTexture(GL_TEXTURE1);
    gpuCache.height.bind();
    glActiveTexture(GL_TEXTURE2);
    gpuCache.imagery.bind();
    CHECK_GLA

    if (SETTINGS->decorateVectorArt)
    {
        glEnable(GL_TEXTURE_1D);

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
#if COVERAGE_PROJECTION_IN_GL
    vp += "uniform mat4 transform;\n";
#endif //COVERAGE_PROJECTION_IN_GL
    vp += "void main()\n{\n";
#if COVERAGE_PROJECTION_IN_GL
    vp += "gl_Position = transform * gl_Vertex;\n";
#else
    vp += "gl_Position = gl_Vertex;\n";
#endif //COVERAGE_PROJECTION_IN_GL
    vp += "gl_FrontColor = gl_Color;\n}\n";

    std::string fp = "void main()\n{\ngl_FragColor = gl_Color;\n}\n";

    lineCoverageShader.addString(GL_VERTEX_SHADER_ARB, vp);
    lineCoverageShader.addString(GL_FRAGMENT_SHADER_ARB, fp);
    lineCoverageShader.link();

#if COVERAGE_PROJECTION_IN_GL
    lineCoverageShader.push();
    lineCoverageTransformUniform = lineCoverageShader.getUniformLocation(
        "transform");
    lineCoverageShader.pop();
#endif //COVERAGE_PROJECTION_IN_GL
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


HitResult QuadTerrain::
intersectNode(const MainBuffer& nodeBuf, const Ray& ray,
              Scalar tin, int sin, Scalar& tout, int& sout,
              const Scalar gout) const
{
    MainData mainData = DATAMANAGER->getData(nodeBuf);
    const NodeData& node = *mainData.node;

//- determine the exit point and side
    tout = Math::Constants<Scalar>::max;
    const Point3* corners[4][2] = {
        {&node.scope.corners[3], &node.scope.corners[2]},
        {&node.scope.corners[2], &node.scope.corners[0]},
        {&node.scope.corners[0], &node.scope.corners[1]},
        {&node.scope.corners[1], &node.scope.corners[3]}};

#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
{
CrustaVisualizer::addScope(node.scope);
CrustaVisualizer::addHit(ray, HitResult(tin), 8);
if (sin!=-1)
{
    Point3s verts;
    verts.resize(2);
    verts[0] = *(corners[sin][0]);
    verts[1] = *(corners[sin][1]);
    CrustaVisualizer::addPrimitive(GL_LINES, verts, -1, Color(0.4, 0.7, 0.8, 1.0));
//    CrustaVisualizer::addPrimitive(GL_LINES, verts, 6, Color(0.4, 0.7, 0.8, 1.0));
}
//construct the corners of the current cell
int tileRes = TILE_RESOLUTION;
QuadNodeMainData::Vertex* cellV = node.geometry;
const QuadNodeMainData::Vertex::Position* positions[4] = {
    &(cellV->position), &((cellV+tileRes-1)->position),
    &((cellV+(tileRes-1)*tileRes)->position), &((cellV+(tileRes-1)*tileRes + tileRes-1)->position) };
Vector3 cellCorners[4];
for (int i=0; i<4; ++i)
{
    for (int j=0; j<3; ++j)
        cellCorners[i][j] = (*(positions[i]))[j] + node.centroid[j];
    Vector3 extrude(cellCorners[i]);
    extrude.normalize();
    extrude *= node.elevationRange[0] * crusta->getVerticalScale();
    cellCorners[i] += extrude;
}
CrustaVisualizer::addTriangle(Triangle(cellCorners[0], cellCorners[3], cellCorners[2]), 4, Color(0.9, 0.6, 0.7, 1.0));
CrustaVisualizer::addTriangle(Triangle(cellCorners[0], cellCorners[1], cellCorners[3]), 3, Color(0.7, 0.6, 0.9, 1.0));
#if DEBUG_INTERSECT_PEEK
CrustaVisualizer::peek();
#endif //DEBUG_INTERSECT_PEEK
CrustaVisualizer::show("Entered new node");
}
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP

    for (int i=0; i<4; ++i)
    {
        if (sin==-1 || i!=sin)
        {
            Section section(*(corners[i][0]), *(corners[i][1]));
#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
#if DEBUG_INTERSECT_SIDES
CrustaVisualizer::addSection(section, 5);
#if DEBUG_INTERSECT_PEEK
CrustaVisualizer::peek();
#endif //DEBUG_INTERSECT_PEEK
#endif //DEBUG_INTERSECT_SIDES
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP
            HitResult hit = section.intersectRay(ray);
            Scalar hitParam  = hit.getParameter();
            if (hit.isValid() && hitParam>tin && hitParam<=tout)
            {
#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
CrustaVisualizer::addHit(ray, hitParam, 7, Color(0.3, 1.0, 0.1, 1.0));
#if DEBUG_INTERSECT_PEEK
CrustaVisualizer::peek();
#endif //DEBUG_INTERSECT_PEEK
CrustaVisualizer::show("Exit search on node");
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP
                tout = hitParam;
                sout = i;
            }
        }
    }
#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
#if DEBUG_INTERSECT_SIDES
CrustaVisualizer::clear(5);
#endif //DEBUG_INTERSECT_SIDES
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP

    const Scalar& verticalScale = crusta->getVerticalScale();

//- check intersection with upper boundary
    Sphere shell(Point3(0), SETTINGS->globeRadius +
                 verticalScale*node.elevationRange[1]);
    Scalar t0, t1;
    bool intersects = shell.intersectRay(ray, t0, t1);

///\todo check lower boundary?

    //does it intersect
    if (!intersects || t0>tout || t1<tin)
        return HitResult();

//- perform leaf intersection?
    //is it even possible to retrieve higher res data?
    if (DATAMANAGER->existsChildData(mainData))
    {
        return intersectLeaf(mainData, ray, tin, sin, gout);
    }

//- determine starting child
    int childId   = -1;
    int leftRight = 0;
    int upDown    = 0;
    switch (sin)
    {
        case -1:
        {
            Point3 mids    = Geometry::mid(*(corners[2][0]), *(corners[2][1]));
            Point3 mide    = Geometry::mid(*(corners[0][0]), *(corners[0][1]));
            Vector3 normal = Geometry::cross(Vector3(mids[0],mids[1],mids[2]),
                                             Vector3(mide[0],mide[1],mide[2]));

            Point3 p = ray(tin);
            Vector3 vp(p[0], p[1], p[2]);
            leftRight = vp*normal>Scalar(0) ? 0 : 1;

            mids   = Geometry::mid(*(corners[3][0]), *(corners[3][1]));
            mide   = Geometry::mid(*(corners[1][0]), *(corners[1][1]));
            normal = Geometry::cross(Vector3(mids[0],mids[1],mids[2]),
                                             Vector3(mide[0],mide[1],mide[2]));

            upDown = vp*normal>Scalar(0) ? 0 : 2;
            break;
        }

        case 0:
        case 2:
        {
            Point3 mids    = Geometry::mid(*(corners[2][0]), *(corners[2][1]));
            Point3 mide    = Geometry::mid(*(corners[0][0]), *(corners[0][1]));
            Vector3 normal = Geometry::cross(Vector3(mids[0],mids[1],mids[2]),
                                             Vector3(mide[0],mide[1],mide[2]));
            Point3 p = ray(tin);
            Vector3 vp(p[0], p[1], p[2]);
            leftRight = vp*normal>Scalar(0) ? 0 : 1;

            upDown = sin==2 ? 0 : 2;
            break;
        }

        case 1:
        case 3:
        {
            leftRight = sin==1 ? 0 : 1;

            Point3 mids    = Geometry::mid(*(corners[3][0]), *(corners[3][1]));
            Point3 mide    = Geometry::mid(*(corners[1][0]), *(corners[1][1]));
            Vector3 normal = Geometry::cross(Vector3(mids[0],mids[1],mids[2]),
                                             Vector3(mide[0],mide[1],mide[2]));
            Point3 p = ray(tin);
            Vector3 vp(p[0], p[1], p[2]);
            upDown = vp*normal>Scalar(0) ? 0 : 2;
            break;
        }

        default:
            assert(false);
    }

    childId = leftRight | upDown;

//- continue traversal
    TreeIndex childIndex = node.index.down(childId);
    MainBuffer childBuf;
    bool childExists = DATAMANAGER->find(childIndex, childBuf);

    Scalar ctin  = tin;
    Scalar ctout = Scalar(0);
    int    csin  = sin;
    int    csout = -1;

#if DEBUG_INTERSECT_CRAP
int childrenVisited = 0;
#endif //DEBUG_INTERSECT_CRAP

    while (true)
    {
        if (childExists)
        {
            //recurse
            HitResult hit = intersectNode(childBuf, ray, ctin, csin,
                                          ctout, csout, gout);
            if (hit.isValid())
                return hit;
            else
            {
                ctin = ctout;
                if (ctin > gout)
                    return HitResult();

#if DEBUG_INTERSECT_CRAP
int oldChildId = childId;
int oldCsin    = csin;
#endif //DEBUG_INTERSECT_CRAP

                //move to the next child
                static const int next[4][4][2] = {
                    { { 2, 2}, {-1,-1}, {-1,-1}, { 1, 1} },
                    { { 3, 2}, { 0, 3}, {-1,-1}, {-1,-1} },
                    { {-1,-1}, {-1,-1}, { 0, 0}, { 3, 1} },
                    { {-1,-1}, { 2, 3}, { 1, 0}, {-1,-1} } };
                csin    = next[childId][csout][1];
                childId = next[childId][csout][0];
                if (childId == -1)
                    return HitResult();

#if DEBUG_INTERSECT_CRAP
MainCacheBuffer* oldBuf = childBuf;
#endif //DEBUG_INTERSECT_CRAP

                childIndex  = node.index.down(childId);
                childExists = DATAMANAGER->find(childIndex, childBuf);

#if DEBUG_INTERSECT_CRAP
++childrenVisited;
if (childExists)
{
    Scalar E = 0.00001;
    int sides[4][2] = {{3,2}, {2,0}, {0,1}, {1,3}};
    const Scope& oldS = oldBuf->getData().scope;
    const Scope& newS = childBuf->getData().scope;
    assert(Geometry::dist(oldS.corners[sides[csout][0]], newS.corners[sides[csin][1]])<E);
    assert(Geometry::dist(oldS.corners[sides[csout][1]], newS.corners[sides[csin][0]])<E);
    std::cerr << "visited children: " << childrenVisited << std::endl;
}
#endif //DEBUG_INTERSECT_CRAP
            }
        }
        else
        {
///\todo Vis2010 simplify. Don't allow loads of nodes from here
//            mainCache.request(MainCache::Request(0.0, nodeBuf, childId));
            return intersectLeaf(mainData, ray, tin, sin, gout);
        }
    }

    //execution should never reach this point
    assert(false);
    return HitResult();
}

HitResult QuadTerrain::
intersectLeaf(const MainData& leafData, const Ray& ray,
              Scalar param, int side, const Scalar gout) const
{
    NodeData& leaf = *leafData.node;

#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
CrustaVisualizer::addScope(leaf.scope, -1, Color(1,0,0,1));
#if DEBUG_INTERSECT_PEEK
CrustaVisualizer::peek();
#endif //DEBUG_INTERSECT_PEEK
CrustaVisualizer::show("Traversing leaf node");
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP

//- locate the cell intersected on the boundary
    int tileRes = TILE_RESOLUTION;
    int cellX = 0;
    int cellY = 0;
    if (side == -1)
    {
        Point3 pos = ray(param);

        int numLevels = 1;
        int res = tileRes-1;
        while (res > 1)
        {
            ++numLevels;
            res >>= 1;
        }

///\todo optimize the containement checks by using vert/horiz split
        Scope scope   = leaf.scope;
        cellX         = 0;
        cellY         = 0;
        int shift     = (tileRes-1) >> 1;
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
                    //adjust the current bottom-left offset
                    cellX += i&0x1 ? shift : 0;
                    cellY += i&0x2 ? shift : 0;
                    shift >>= 1;
                    //switch to that scope
                    scope = childScopes[i];
                    break;
                }
            }
        }
#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
//** verify cell
{
Scalar verticalScale = crusta->getVerticalScale();
int offset = cellY*tileRes + cellX;
QuadNodeMainData::Vertex* cellV = leaf.geometry + offset;
DemHeight*                cellH = leaf.height   + offset;

const QuadNodeMainData::Vertex::Position* positions[4] = {
    &(cellV->position), &((cellV+1)->position),
    &((cellV+tileRes)->position), &((cellV+tileRes+1)->position) };
const DemHeight* heights[4] = {
    cellH, cellH+1, cellH+tileRes, cellH+tileRes+1
};
//construct the corners of the current cell
Vector3 cellCorners[4];
for (int i=0; i<4; ++i)
{
    for (int j=0; j<3; ++j)
        cellCorners[i][j] = (*(positions[i]))[j] + leaf.centroid[j];
    Vector3 extrude(cellCorners[i]);
    extrude.normalize();
    extrude *= *(heights[i]) * verticalScale;
    cellCorners[i] += extrude;
}

//determine the exit point and side
const Vector3* segments[4][2] = {
    {&(cellCorners[3]), &(cellCorners[2])},
    {&(cellCorners[2]), &(cellCorners[0])},
    {&(cellCorners[0]), &(cellCorners[1])},
    {&(cellCorners[1]), &(cellCorners[3])} };
Scalar oldParam = param;
int    oldSide  = side;
int    newParam = Math::Constants<Scalar>::max;
bool   badEntry = false;
for (int i=0; i<4; ++i)
{
    Section section(*(segments[i][0]), *(segments[i][1]));
    HitResult hit   = section.intersectRay(ray);
    Scalar hitParam = hit.getParameter();
    if (i != oldSide)
    {
        if (hit.isValid() && hitParam>=oldParam && hitParam<=newParam)
            newParam = hitParam;
    }
    else
    {
        if (!hit.isValid() || Math::abs(hitParam-param)>Scalar(0.0001))
        {
            std::cerr << "hit is: " << hit.isValid() << std::endl <<
                      "hitParam " << hitParam << " param " << param <<
                      " diff " << Math::abs(hitParam-param) << std::endl;
            badEntry = true;
        }
    }
}
if (badEntry || newParam == Math::Constants<Scalar>::max)
{
    CrustaVisualizer::addScope(leaf.scope);
    CrustaVisualizer::addRay(ray);
    CrustaVisualizer::addHit(ray, HitResult(oldParam));
    CrustaVisualizer::addTriangle(Triangle(cellCorners[0], cellCorners[3], cellCorners[2]), -1, Color(0.9, 0.6, 0.7, 1.0));
    CrustaVisualizer::addTriangle(Triangle(cellCorners[0], cellCorners[1], cellCorners[3]), -1, Color(0.7, 0.6, 0.9, 1.0));
    for (int i=0; i<4; ++i)
        CrustaVisualizer::addSection(Section(*(segments[i][0]), *(segments[i][1])));
    CrustaVisualizer::show("Bad cell entry");
}
}
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP
    }
    else
    {
        int corners[4][2] = { {2,3}, {0,2}, {0,1}, {1,3} };
        Section entryEdge(leaf.scope.corners[corners[side][0]],
                          leaf.scope.corners[corners[side][1]]);
        Point3 entryPoint   = ray(param);
        HitResult alongEdge = entryEdge.intersectWithSegment(entryPoint);
#if DEBUG_INTERSECT_CRAP
if (!(alongEdge.isValid() &&
      alongEdge.getParameter()>=0 && alongEdge.getParameter()<=1.0))
{
CrustaVisualizer::addScope(leaf.scope);
CrustaVisualizer::addSection(entryEdge);
CrustaVisualizer::addRay(ray);
CrustaVisualizer::addHit(ray, HitResult(param));
CrustaVisualizer::show("Busted Entry");
}
#endif //DEBUG_INTERSECT_CRAP
///\todo Vis2010 just bail here. Need to figure out how this can be happening
#if 1
if (!alongEdge.isValid() ||
    alongEdge.getParameter()<0 || alongEdge.getParameter()>1.0)
{
    return HitResult();
}
#else
        assert(alongEdge.isValid() &&
               alongEdge.getParameter()>=0 && alongEdge.getParameter()<=1.0);
#endif

        int edgeIndex = alongEdge.getParameter() * (tileRes-1);
        if (edgeIndex == tileRes-1)
            --edgeIndex;

        switch (side)
        {
            case 0:  cellX = edgeIndex; cellY = tileRes-2; break;
            case 1:  cellX = 0;         cellY = edgeIndex; break;
            case 2:  cellX = edgeIndex; cellY = 0;         break;
            case 3:  cellX = tileRes-2; cellY = edgeIndex; break;
            default: cellX = 0;         cellY = 0;         assert(0);
        }

#if DEBUG_INTERSECT_CRAP
//** verify cell
if (DEBUG_INTERSECT) {
{
Scalar verticalScale = crusta->getVerticalScale();
int offset = cellY*tileRes + cellX;
QuadNodeMainData::Vertex* cellV = leaf.geometry + offset;
DemHeight*                cellH = leaf.height   + offset;

const QuadNodeMainData::Vertex::Position* positions[4] = {
    &(cellV->position), &((cellV+1)->position),
    &((cellV+tileRes)->position), &((cellV+tileRes+1)->position) };
const DemHeight* heights[4] = {
    cellH, cellH+1, cellH+tileRes, cellH+tileRes+1
};
//construct the corners of the current cell
Vector3 cellCorners[4];
for (int i=0; i<4; ++i)
{
    for (int j=0; j<3; ++j)
        cellCorners[i][j] = (*(positions[i]))[j] + leaf.centroid[j];
    Vector3 extrude(cellCorners[i]);
    extrude.normalize();
    extrude *= *(heights[i]) * verticalScale;
    cellCorners[i] += extrude;
}

//determine the exit point and side
const Vector3* segments[4][2] = {
    {&(cellCorners[3]), &(cellCorners[2])},
    {&(cellCorners[2]), &(cellCorners[0])},
    {&(cellCorners[0]), &(cellCorners[1])},
    {&(cellCorners[1]), &(cellCorners[3])} };
Scalar oldParam = param;
int    oldSide  = side;
int    newParam = Math::Constants<Scalar>::max;
bool   badEntry = false;
for (int i=0; i<4; ++i)
{
    Section section(*(segments[i][0]), *(segments[i][1]));
    HitResult hit   = section.intersectRay(ray);
    Scalar hitParam = hit.getParameter();
    if (i != oldSide)
    {
        if (hit.isValid() && hitParam>=oldParam && hitParam<=newParam)
            newParam = hitParam;
    }
    else
    {
        if (!hit.isValid() || Math::abs(hitParam-param)>Scalar(0.0001))
        {
            std::cerr << "hit is: " << hit.isValid() << std::endl <<
                      "hitParam " << hitParam << " param " << param <<
                      " diff " << Math::abs(hitParam-param) << std::endl;
            badEntry = true;
        }
    }
}
if (badEntry || newParam == Math::Constants<Scalar>::max)
{
    CrustaVisualizer::addScope(leaf.scope);
    CrustaVisualizer::addRay(ray);
    CrustaVisualizer::addRay(Ray(Point3(0,0,0), Section(*(segments[side][0]), *(segments[side][1])).projectOntoPlane(ray(param))), -1, Color(0.1, 0.7, 1.0));
    CrustaVisualizer::addHit(Ray(leaf.scope.corners[corners[side][0]], leaf.scope.corners[corners[side][1]]), alongEdge.getParameter(), -1, Color(0.1, 0.2, 1.0));
    CrustaVisualizer::addHit(ray, HitResult(oldParam));
    CrustaVisualizer::addTriangle(Triangle(cellCorners[0], cellCorners[3], cellCorners[2]), -1, Color(0.9, 0.6, 0.7, 1.0));
    CrustaVisualizer::addTriangle(Triangle(cellCorners[0], cellCorners[1], cellCorners[3]), -1, Color(0.7, 0.6, 0.9, 1.0));
    for (int i=0; i<4; ++i)
    {
        if (i==oldSide)
            CrustaVisualizer::addSection(Section(*(segments[i][0]), *(segments[i][1])),
                                         -1, Color(1.0, 0.3, 0.3, 1.0));
        else
            CrustaVisualizer::addSection(Section(*(segments[i][0]), *(segments[i][1])));
    }
    CrustaVisualizer::show("Bad cell entry");
}
}
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP
    }

//- traverse cells
#if DEBUG_INTERSECT_CRAP
int traversedCells = 0;
#endif //DEBUG_INTERSECT_CRAP
    Scalar verticalScale = crusta->getVerticalScale();
    int offset = cellY*tileRes + cellX;
    Vertex*    cellV = leafData.geometry + offset;
    DemHeight* cellH = leafData.height   + offset;
    while (true)
    {
        const Vertex::Position* positions[4] = {
            &(cellV->position), &((cellV+1)->position),
            &((cellV+tileRes)->position), &((cellV+tileRes+1)->position) };
        const DemHeight* heights[4] = {
            cellH, cellH+1, cellH+tileRes, cellH+tileRes+1
        };
        //construct the corners of the current cell
        Vector3 cellCorners[4];
        for (int i=0; i<4; ++i)
        {
            for (int j=0; j<3; ++j)
                cellCorners[i][j] = (*(positions[i]))[j] + leaf.centroid[j];
            Vector3 extrude(cellCorners[i]);
            extrude.normalize();
            extrude *= *(heights[i]) * verticalScale;
            cellCorners[i] += extrude;
        }

        //intersect triangles of current cell
        Triangle t0(cellCorners[0], cellCorners[3], cellCorners[2]);
        Triangle t1(cellCorners[0], cellCorners[1], cellCorners[3]);

#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
CrustaVisualizer::addTriangle(t0, -1, Color(0.9, 0.6, 0.7, 1.0));
CrustaVisualizer::addTriangle(t1, -1, Color(0.7, 0.6, 0.9, 1.0));
#if DEBUG_INTERSECT_PEEK
CrustaVisualizer::peek();
#endif //DEBUG_INTERSECT_PEEK
CrustaVisualizer::show("Intersecting triangles");
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP

        HitResult hit = t0.intersectRay(ray);
        if (hit.isValid())
        {
#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
{
Point3s verts;
verts.resize(1, ray(hit.getParameter()));
CrustaVisualizer::addPrimitive(GL_POINTS, verts, 2, Color(1));
#if DEBUG_INTERSECT_PEEK
CrustaVisualizer::peek();
#endif //DEBUG_INTERSECT_PEEK
CrustaVisualizer::show("INTERSECTION");
CrustaVisualizer::clear(2);
}
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP
            return hit;
        }
        hit = t1.intersectRay(ray);
        if (hit.isValid())
        {
#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
{
Point3s verts;
verts.resize(1, ray(hit.getParameter()));
CrustaVisualizer::addPrimitive(GL_POINTS, verts, 2, Color(1));
#if DEBUG_INTERSECT_PEEK
CrustaVisualizer::peek();
#endif //DEBUG_INTERSECT_PEEK
CrustaVisualizer::show("INTERSECTION");
CrustaVisualizer::clear(2);
}
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP
            return hit;
        }

        //determine the exit point and side
        const Vector3* segments[4][2] = {
            {&(cellCorners[3]), &(cellCorners[2])},
            {&(cellCorners[2]), &(cellCorners[0])},
            {&(cellCorners[0]), &(cellCorners[1])},
            {&(cellCorners[1]), &(cellCorners[3])} };
        Scalar oldParam = param;
        int    oldSide  = side;
        param           = Math::Constants<Scalar>::max;
        for (int i=0; i<4; ++i)
        {
            if (i != oldSide)
            {
                Section section(*(segments[i][0]), *(segments[i][1]));
                HitResult hit   = section.intersectRay(ray);
                Scalar hitParam = hit.getParameter();
#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
CrustaVisualizer::addSection(section, 5);
if (hit.isValid() && hitParam>=oldParam && hitParam<=param)
    CrustaVisualizer::addHit(ray, hitParam, 7, Color(0.3, 1.0, 0.1, 1.0));
CrustaVisualizer::show("Exit search on cell");
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP
                if (hit.isValid() && hitParam>=oldParam && hitParam<=param)
                {
                    param = hitParam;
                    side  = i;
                }
            }
        }

        //end traversal if we did not find an exit point from the current cell
        if (param == Math::Constants<Scalar>::max)
        {
#if DEBUG_INTERSECT_CRAP
if (DEBUG_INTERSECT) {
CrustaVisualizer::addScope(leaf.scope);
CrustaVisualizer::addRay(ray);
CrustaVisualizer::addHit(ray, HitResult(oldParam));
CrustaVisualizer::addTriangle(t0, -1, Color(0.9, 0.6, 0.7, 1.0));
CrustaVisualizer::addTriangle(t1, -1, Color(0.7, 0.6, 0.9, 1.0));
for (int i=0; i<4; ++i)
{
if (i==oldSide)
    CrustaVisualizer::addSection(Section(*(segments[i][0]), *(segments[i][1])),
                                 -1, Color(1.0, 0.3, 0.3, 1.0));
else
    CrustaVisualizer::addSection(Section(*(segments[i][0]), *(segments[i][1])));
}
std::cerr << "traversedCells: " << traversedCells << std::endl;
//CrustaVisualizer::show("Early exit Cell");
} //DEBUG_INTERSECT
#endif //DEBUG_INTERSECT_CRAP

            return HitResult();
        }

        //move to the next cell
        if (param > gout)
            return HitResult();

        static const int next[4][3] = { {0,1,2}, {-1,0,3}, {0,-1,0}, {1,0,1} };

        cellX += next[side][0];
        cellY += next[side][1];
        if (cellX<0 || cellX>tileRes-2 || cellY<0 || cellY>tileRes-2)
            return HitResult();

        offset = cellY*tileRes + cellX;
        cellV  = leafData.geometry + offset;
        cellH  = leafData.height   + offset;

        side = next[side][2];
#if DEBUG_INTERSECT_CRAP
++traversedCells;
#endif //DEBUG_INTERSECT_CRAP
    }

    return HitResult();
}


void QuadTerrain::
intersectNode(Shape::IntersectionFunctor& callback, const MainBuffer& nodeBuf,
              Ray& ray, Scalar tin, int sin, Scalar& tout, int& sout) const
{
    MainData  nodeData = DATAMANAGER->getData(nodeBuf);
    NodeData& node     = *nodeData.node;

//- continue traversal
    Point3 entry         = ray(tin);
    int childId          = computeContainingChild(entry, sin, node.scope);
    TreeIndex childIndex = node.index.down(childId);
    MainBuffer childBuf;
    bool childExists = DATAMANAGER->find(childIndex, childBuf);

    if (!childExists)
    {
        //callback for the current node
        callback(node, true);
        intersectLeaf(node, ray, tin, sin, tout, sout);
        return;
    }
    else
    {
        //callback for the current node
        callback(node, false);

        //recurse
        while (true)
        {
            intersectNode(callback, childBuf, ray, tin, sin, tout, sout);
            if (tout >= 1.0)
                return;
            tin = tout;

            //move to the next child
            static const int next[4][4][2] = {
                { { 2, 2}, {-1,-1}, {-1,-1}, { 1, 1} },
                { { 3, 2}, { 0, 3}, {-1,-1}, {-1,-1} },
                { {-1,-1}, {-1,-1}, { 0, 0}, { 3, 1} },
                { {-1,-1}, { 2, 3}, { 1, 0}, {-1,-1} } };
            sin     = next[childId][sout][1];
            childId = next[childId][sout][0];
            if (childId == -1)
                return;

            childIndex  = node.index.down(childId);
            childExists = DATAMANAGER->find(childIndex, childBuf);
            assert(childExists);
        }
    }

    //execution should never reach this point
    assert(false);
}

void QuadTerrain::
intersectLeaf(NodeData& leaf, Ray& ray, Scalar tin, int sin,
              Scalar& tout, int& sout) const
{
    const Scope& scope  = leaf.scope;
    Section sections[4] = { Section(scope.corners[3], scope.corners[2]),
                            Section(scope.corners[2], scope.corners[0]),
                            Section(scope.corners[0], scope.corners[1]),
                            Section(scope.corners[1], scope.corners[3]) };

    //compute exit for current segment
    tout = Math::Constants<Scalar>::max;
    for (int i=0; i<4; ++i)
    {
        if (sin==-1 || i!=sin)
        {
            HitResult hit   = sections[i].intersectRay(ray);
            Scalar hitParam = hit.getParameter();
            if (hit.isValid() && hitParam>tin && hitParam<=tout)
            {
                tout = hitParam;
                sout = i;
            }
        }
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

    crustaGl->terrainShader.setCentroid(
        Point3f(main.centroid[0], main.centroid[1], main.centroid[2]));
    crustaGl->terrainShader.setGeometrySubRegion(*gpuData.geometry);
    crustaGl->terrainShader.setHeightSubRegion(*gpuData.height);
    crustaGl->terrainShader.setColorSubRegion(*gpuData.imagery);
    CHECK_GLA

    if (SETTINGS->decorateVectorArt)
    {
        //setup the shader for decorated line drawing
        crustaGl->terrainShader.setLineNumSegments(main.lineNumSegments);
        if (main.lineNumSegments > 0)
        {
            crustaGl->terrainShader.setCoverageSubRegion(*gpuData.coverage);
            crustaGl->terrainShader.setLineDataSubRegion(
                (SubRegion)(*gpuData.lineData));
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
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);

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
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);

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
if (allgood && data.node->lineCoverageDirty)
{
    for (int i=0; i<4; ++i)
    {
        NodeMainData child = DATAMANAGER->getData(children[i]);
CRUSTA_DEBUG(60, std::cerr << "***COVDOWN parent(" << data.node->index <<
")    " << "n(" << data.node->index << ")\n\n";)
        mapMan->inheritShapeCoverage(*data.node, *child.node);
    }
    //reset the dirty flag
    data.node->lineCoverageDirty = false;
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
            Ray ray((*hit)->pos, end->pos);
            Scalar tin, tout;
            int sin, sout;
            intersectNodeSides(node.scope, ray, tin, sin, tout, sout);
            assert(tin<1.0 && tout>0.0);
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
