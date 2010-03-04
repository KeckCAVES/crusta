#include <crusta/CrustaVisualizer.h>

#include <crusta/Scope.h>
#include <crusta/Section.h>
#include <crusta/Triangle.h>


BEGIN_CRUSTA


Color CrustaVisualizer::defaultScopeColor(1);
Color CrustaVisualizer::defaultSectionColor(0.7, 0.8, 1.0, 1.0);
Color CrustaVisualizer::defaultTriangleColor(0.7, 0.6, 0.9, 1.0);
Color CrustaVisualizer::defaultRayColor(0.3, 0.8, 0.4, 1.0);
Color CrustaVisualizer::defaultHitColor(0.9, 0.3, 0.2, 1.0);

void CrustaVisualizer::
addScope(const Scope& s, int temp, const Color& color)
{
    Primitive& newPrim = vis->getNewPrimitive(temp);

    newPrim.mode  = GL_LINE_STRIP;
    newPrim.color = color;

    Point3s verts;
    verts.resize(5);
    verts[0] = s.corners[0];
    verts[1] = s.corners[1];
    verts[2] = s.corners[3];
    verts[3] = s.corners[2];
    verts[4] = s.corners[0];

    newPrim.setVertices(verts);
}

void CrustaVisualizer::
addSection(const Section& s, int temp, const Color& color)
{
    Primitive& newPrim = vis->getNewPrimitive(temp);

    newPrim.mode  = GL_TRIANGLES;
    newPrim.color = color;

    Scalar x = 1.002;
    Point3s verts;
    verts.resize(3);
    verts[0] = Point3(0);
    verts[1] = Point3(x*s.getStart()[0], x*s.getStart()[1], x*s.getStart()[2]);
    verts[2] = Point3(x*s.getEnd()[0], x*s.getEnd()[1], x*s.getEnd()[2]);

    newPrim.setVertices(verts);
}


void CrustaVisualizer::
addTriangle(const Triangle& t, int temp, const Color& color)
{
    Primitive& newPrim = vis->getNewPrimitive(temp);

    newPrim.mode  = GL_TRIANGLES;
    newPrim.color = color;

    Point3s verts;
    verts.resize(3);
    verts[0] = t.getVert0();
    verts[1] = verts[0] + t.getEdge1();
    verts[2] = verts[0] + t.getEdge2();

    newPrim.setVertices(verts);
}

void CrustaVisualizer::
addRay(const Ray& r, int temp, const Color& color)
{
    Primitive& newPrim = vis->getNewPrimitive(temp);

    newPrim.mode  = GL_LINES;
    newPrim.color = color;

    Point3s verts;
    verts.resize(2);
    verts[0] = r.getOrigin();
    verts[1] = r(0.00001 * SPHEROID_RADIUS);

    newPrim.setVertices(verts);
}

void CrustaVisualizer::
addHit(const Ray& r, const HitResult& h, int temp, const Color& color)
{
    if (!h.isValid())
        return;

    Primitive& newPrim = vis->getNewPrimitive(temp);

    newPrim.mode  = GL_POINTS;
    newPrim.color = color;

    Point3s verts;
    verts.resize(1);
    verts[0] = r(h.getParameter());

    newPrim.setVertices(verts);
}


void CrustaVisualizer::
resetNavigationCallback(Misc::CallbackData* cbData)
{
    /* Reset the Vrui navigation transformation: */
    Vrui::setNavigationTransformation(Vrui::Point(0,0,0), 1.5*SPHEROID_RADIUS);
}


END_CRUSTA
