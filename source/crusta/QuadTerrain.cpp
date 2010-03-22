#include <crusta/QuadTerrain.h>

#include <algorithm>
#include <assert.h>

#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/DisplayState.h>
#include <Vrui/ViewSpecification.h>
#include <Vrui/Vrui.h>
#include <Vrui/VRWindow.h>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>
#include <crusta/DataManager.h>
#include <crusta/LightingShader.h>
#include <crusta/map/MapManager.h>
#include <crusta/QuadCache.h>
#include <crusta/Triangle.h>
#include <crusta/Section.h>
#include <crusta/Sphere.h>

#if DEBUG_INTERSECT_CRAP
#define DEBUG_INTERSECT_SIDES 1
#define DEBUG_INTERSECT_PEEK 0
#endif //DEBUG_INTERSECT_CRAP
#include <crusta/CrustaVisualizer.h>
#define CV(x) CrustaVisualizer::x

///\todo used for debugspheres
#include <GL/GLModels.h>

BEGIN_CRUSTA

static const uint NUM_GEOMETRY_INDICES =
    (TILE_RESOLUTION-1)*(TILE_RESOLUTION*2 + 2) - 2;
static const float TEXTURE_COORD_START = TILE_TEXTURE_COORD_STEP * 0.5;
static const float TEXTURE_COORD_END   = 1.0 - TEXTURE_COORD_START;

bool QuadTerrain::displayDebuggingBoundingSpheres = false;
bool QuadTerrain::displayDebuggingGrid            = false;

QuadTerrain::
QuadTerrain(uint8 patch, const Scope& scope, Crusta* iCrusta) :
    CrustaComponent(iCrusta), rootIndex(patch)
{
    crusta->getDataManager()->loadRoot(rootIndex, scope);
}


const QuadNodeMainData& QuadTerrain::
getRootNode() const
{
    MainCacheBuffer* root =
        crusta->getCache()->getMainCache().findCached(rootIndex);
    assert(root != NULL);

    return root->getData();
}

HitResult QuadTerrain::
intersect(const Ray& ray, Scalar tin, int sin, Scalar& tout, int& sout,
          const Scalar gout) const
{
    MainCacheBuffer* nodeBuf =
        crusta->getCache()->getMainCache().findCached(rootIndex);
    assert(nodeBuf != NULL);

    return intersectNode(nodeBuf, ray, tin, sin, tout, sout, gout);
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
intersect(Shape::ControlPointHandle& start,
    const Shape::ControlPointHandle& end, Shape::IntersectionFunctor& callback,
    Ray& ray, Scalar tin, int sin, Scalar& tout, int& sout) const
{
    MainCache& mainCache     = crusta->getCache()->getMainCache();
    MainCacheBuffer* nodeBuf = mainCache.findCached(rootIndex);
    assert(nodeBuf != NULL);

    intersectNode(start, end, callback, nodeBuf, ray, tin, sin, tout, sout);
}


void QuadTerrain::
intersectNodeSides(const QuadNodeMainData& node, const Ray& ray,
                   Scalar& tin, int& sin, Scalar& tout, int& sout)
{
    const Scope& scope  = node.scope;
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
            else if (hitParam > tout)
            {
                tout = hitParam;
                sout = i;
            }
        }
    }
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
prepareDisplay(GLContextData& contextData, Nodes& nodes)
{
    //setup the evaluators
    FrustumVisibility visibility;
    visibility.frustum = getFrustumFromVrui(contextData);
    FocusViewEvaluator lod;
    lod.frustum = visibility.frustum;
    lod.setFocusFromDisplay();

    /* display could be multi-threaded. Buffer all the node data requests and
       merge them into the request list en block */
    MainCache::Requests dataRequests;
    /* as for requests for new data, buffer all the active node sets and submit
       them at the end */
    NodeBufs actives;

    /* traverse the terrain tree, update as necessary and issue drawing commands
       for active nodes */
    MainCacheBuffer* rootBuf =
        crusta->getCache()->getMainCache().findCached(rootIndex);
    assert(rootBuf != NULL);

    prepareDraw(visibility, lod, rootBuf, actives, nodes, dataRequests);

    //merge the data requests and active sets
    crusta->getCache()->getMainCache().request(dataRequests);
    crusta->submitActives(actives);
}

void QuadTerrain::
display(GLContextData& contextData, CrustaGlData* glData, Nodes& nodes)
{
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

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);

    /* save the current openGL transform. We are going to replace it during
       traversal with a scope centroid centered one to alleviate floating point
       issues with rotating vertices far off the origin */
    glPushMatrix();

    int numNodes = static_cast<int>(nodes.size());
    for (int i=0; i<numNodes; ++i)
        drawNode(contextData, glData, *(nodes[i]));

    //restore the GL transform as it was before
    glPopMatrix();

    glPopClientAttrib();
    glPopAttrib();

    glActiveTexture(activeTexture);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
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
    glGenBuffers(1, &vertexAttributeTemplate);
    glBindBuffer(GL_ARRAY_BUFFER, vertexAttributeTemplate);
    glBufferData(GL_ARRAY_BUFFER, numTexCoords*sizeof(float), positionsInMemory,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

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
    glGenBuffers(1, &indexTemplate);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexTemplate);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, NUM_GEOMETRY_INDICES*sizeof(uint16),
                 indicesInMemory, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //clean-up
    delete[] indicesInMemory;
}


HitResult QuadTerrain::
intersectNode(MainCacheBuffer* nodeBuf, const Ray& ray,
              Scalar tin, int sin, Scalar& tout, int& sout,
              const Scalar gout) const
{
    const QuadNodeMainData& node = nodeBuf->getData();

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
    Sphere shell(Point3(0), SPHEROID_RADIUS +
                 verticalScale*node.elevationRange[1]);
    Scalar t0, t1;
    bool intersects = shell.intersectRay(ray, t0, t1);

///\todo check lower boundary?

    //does it intersect
    if (!intersects || t0>tout || t1<tin)
        return HitResult();

//- perform leaf intersection?
    //is it even possible to retrieve higher res data?
    if (node.childDemTiles[0]   ==   DemFile::INVALID_TILEINDEX &&
        node.childColorTiles[0] == ColorFile::INVALID_TILEINDEX)
    {
        return intersectLeaf(node, ray, tin, sin, gout);
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
    MainCache& mainCache      = crusta->getCache()->getMainCache();
    TreeIndex childIndex      = node.index.down(childId);
    MainCacheBuffer* childBuf = mainCache.findCached(childIndex);

    Scalar ctin  = tin;
    Scalar ctout = Scalar(0);
    int    csin  = sin;
    int    csout = -1;

#if DEBUG_INTERSECT_CRAP
int childrenVisited = 0;
#endif //DEBUG_INTERSECT_CRAP

    while (true)
    {
        if (childBuf == NULL)
        {
            mainCache.request(MainCache::Request(0.0, nodeBuf, childId));
            return intersectLeaf(node, ray, tin, sin, gout);
        }
        else
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

                childIndex = node.index.down(childId);
                childBuf   = mainCache.findCached(childIndex);

#if DEBUG_INTERSECT_CRAP
++childrenVisited;
if (childBuf != NULL)
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
    }

    //execution should never reach this point
    assert(false);
    return HitResult();
}

HitResult QuadTerrain::
intersectLeaf(const QuadNodeMainData& leaf, const Ray& ray,
              Scalar param, int side, const Scalar gout) const
{
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
        assert(alongEdge.isValid() &&
               alongEdge.getParameter()>=0 && alongEdge.getParameter()<=1.0);

        int edgeIndex = alongEdge.getParameter() * (tileRes-1);
        if (edgeIndex == tileRes-1)
            --edgeIndex;

        switch (side)
        {
            case 0:  cellX = edgeIndex; cellY = tileRes-2; break;
            case 1:  cellX = 0;         cellY = edgeIndex; break;
            case 2:  cellX = edgeIndex; cellY = 0;         break;
            case 3:  cellX = tileRes-2; cellY = edgeIndex; break;
            default: assert(false);
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
    QuadNodeMainData::Vertex* cellV = leaf.geometry + offset;
    DemHeight*                cellH = leaf.height   + offset;
    while (true)
    {
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
        cellV  = leaf.geometry + offset;
        cellH  = leaf.height   + offset;

        side = next[side][2];
#if DEBUG_INTERSECT_CRAP
++traversedCells;
#endif //DEBUG_INTERSECT_CRAP
    }

    return HitResult();
}


void QuadTerrain::
intersectNode(Shape::ControlPointHandle& start,
    const Shape::ControlPointHandle& end, Shape::IntersectionFunctor& callback,
    MainCacheBuffer* nodeBuf, Ray& ray, Scalar tin, int sin, Scalar& tout,
    int& sout) const
{
    QuadNodeMainData& node = nodeBuf->getData();

CRUSTA_DEBUG(20,
CV(addScope(node.scope));
CV(addHit(ray, HitResult(tin), 8));
CV(addSideIn(sin, node.scope));
CV(show("Entered new node"));
)

//- perform leaf intersection?
    //is it even possible to retrieve higher res data?
    if (node.childDemTiles[0]   ==   DemFile::INVALID_TILEINDEX &&
        node.childColorTiles[0] == ColorFile::INVALID_TILEINDEX)
    {
        intersectLeaf(start, end, callback, node, ray, tin, sin, tout, sout);
        return;
    }

//- continue traversal
    Point3 entry              = ray(tin);
    int childId               = computeContainingChild(entry, sin, node.scope);
    MainCache& mainCache      = crusta->getCache()->getMainCache();
    TreeIndex childIndex      = node.index.down(childId);
    MainCacheBuffer* childBuf = mainCache.findCached(childIndex);

    while (true)
    {
        if (childBuf==NULL || !mainCache.isActive(childBuf))
        {
            intersectLeaf(start, end, callback, node, ray, tin,sin, tout,sout);
            return;
        }
        else
        {
            //recurse
            intersectNode(start, end, callback, childBuf, ray, tin, sin,
                          tout, sout);
            if (start == end)
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

            childIndex = node.index.down(childId);
            childBuf   = mainCache.findCached(childIndex);
        }
    }

    //execution should never reach this point
    assert(false);
}

void QuadTerrain::
intersectLeaf(Shape::ControlPointHandle& start,
    const Shape::ControlPointHandle& end, Shape::IntersectionFunctor& callback,
    QuadNodeMainData& leaf, Ray& ray, Scalar tin, int sin, Scalar& tout,
    int& sout) const
{
    const Scope& scope  = leaf.scope;
    Section sections[4] = { Section(scope.corners[3], scope.corners[2]),
                            Section(scope.corners[2], scope.corners[0]),
                            Section(scope.corners[0], scope.corners[1]),
                            Section(scope.corners[1], scope.corners[3]) };
    //call back for all the nodes of the polyline that are contained
    while (true)
    {
        //call back for the current control point
        callback(start, &leaf);

        //compute exit for current segment
        tout = Math::Constants<Scalar>::max;
        for (int i=0; i<4; ++i)
        {
CRUSTA_DEBUG(21,
CV(addSection(sections[i], 5));
CV(peek());
)
            if (sin==-1 || i!=sin)
            {
                HitResult hit   = sections[i].intersectRay(ray);
                Scalar hitParam = hit.getParameter();
                if (hit.isValid() && hitParam>tin && hitParam<=tout)
                {
CRUSTA_DEBUG(21,
CV(addHit(ray, hit, 7, Color(0.3, 1.0, 0.1, 1.0)));
CV(show("Exit found"));
)
                    tout = hitParam;
                    sout = i;
                }
            }
        }
CRUSTA_DEBUG(21,
CV(clear(5));
)

        //try to move out of the leaf
        if (tout >= 1.0)
        {
            //move to the next segment and loop
            tin =  0.0;
            sin = -1.0;
            ++start;
            Shape::ControlPointHandle next = start;
            ++next;
            if (next == end)
            {
                start = end;
                break;
            }
            ray = Ray(start->pos, next->pos);
        }
        else
            break;
    }
}

const QuadNodeVideoData& QuadTerrain::
prepareGlData(CrustaGlData* glData, QuadNodeMainData& mainData)
{
    bool existed;
    VideoCacheBuffer* videoBuf = glData->videoCache->getBuffer(mainData.index,
                                                               &existed);
    if (existed)
    {
        //if there was already a match in the cache, just use that data
        glData->videoCache->touch(videoBuf);
        return videoBuf->getData();
    }
    else
    {
        //in any case the data has to be transfered from main memory
        if (videoBuf)
            glData->videoCache->touch(videoBuf);
        else
            videoBuf = glData->videoCache->getStreamBuffer();

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
drawNode(GLContextData& contextData, CrustaGlData* glData,
         QuadNodeMainData& mainData)
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
    CHECK_GLA

///\todo integrate me properly into the system (VIS 2010)
//stream the line data to the GPU
if (mainData.lineData.empty())
    glData->terrainShader.setLineStartCoord(0.0);
else
{
    glData->terrainShader.setLineStartCoord(Crusta::lineDataStartCoord);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_1D, glData->lineDataTex);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, mainData.lineData.size(), GL_RGBA,
                    GL_FLOAT, mainData.lineData.front().getComponents());
}

#if 1
    //load the centroid relative translated navigation transformation
    glPushMatrix();
    Vrui::Vector centroidTranslation(
        mainData.centroid[0], mainData.centroid[1], mainData.centroid[2]);
    Vrui::NavTransform nav =
    Vrui::getDisplayState(contextData).modelviewNavigational;
    nav *= Vrui::NavTransform::translate(centroidTranslation);
    glLoadMatrix(nav);

    glData->terrainShader.setCentroid(
        mainData.centroid[0], mainData.centroid[1], mainData.centroid[2]);

    CHECK_GLA
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
    CHECK_GLA
#endif

if (displayDebuggingBoundingSpheres)
{
glData->terrainShader.disable();
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
glData->terrainShader.enable();
}

if (displayDebuggingGrid)
{
    CHECK_GLA
glData->terrainShader.disable();
    CHECK_GLA
    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    CHECK_GLA
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
    CHECK_GLA

    glPopAttrib();
    glActiveTexture(activeTexture);
    CHECK_GLA
glData->terrainShader.enable();
    CHECK_GLA
}

}

void QuadTerrain::
prepareDraw(FrustumVisibility& visibility, FocusViewEvaluator& lod,
            MainCacheBuffer* node, NodeBufs& actives, Nodes& renders,
            MainCache::Requests& requests)
{
    MainCache&  mainCache = crusta->getCache()->getMainCache();
    MapManager* mapMan    = crusta->getMapManager();

    //confirm current node as being active
    actives.push_back(node);
    mainCache.touch(node);

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
                        requests.push_back(MainCache::Request(lodValue,node,i));
                        allgood = false;
                    }
                }
            }
            //check that all the children are current
            if (allgood)
            {
                for (int i=0; i<4; ++i)
                {
                    MainCacheBuffer* childBuf = children[i];
                    if (!mainCache.isValid(childBuf))
                    {
                        allgood = false;
                    }
                    else if (!mainCache.isCurrent(childBuf))
                    {
                        //"request" it for update
                        actives.push_back(childBuf);
                        mainCache.touch(childBuf);
                        allgood = false;
                    }
                }
            }
/**\todo horrible Vis2010 HACK: integrate this in the proper way? I.e. don't
stall here, but defer the update. */
if (allgood)
{
    for (int i=0; i<4; ++i)
    {
        QuadNodeMainData& child = children[i]->getData();
        if (child.lineCoverageAge != mainData.lineCoverageAge)
        {
CRUSTA_DEBUG(60, std::cerr << "***COVUP n(" << child.index << ") - ages " <<
child.lineCoverageAge << " vs " << mainData.lineCoverageAge << "\n\n";)
            child.lineCoverage.clear();
            mapMan->inheritShapeCoverage(mainData, child);
        }
    }
}

            //still all good then recurse to the children
            if (allgood)
            {
                for (int i=0; i<4; ++i)
                {
                    prepareDraw(visibility, lod, children[i], actives, renders,
                                requests);
                }
            }
            else
                renders.push_back(&mainData);
        }
        else
            renders.push_back(&mainData);
    }
}


END_CRUSTA
