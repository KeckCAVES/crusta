#include <crusta/CrustaVisualizer.h>

#include <crustacore/Scope.h>
#include <crustacore/Section.h>
#include <crusta/Triangle.h>


BEGIN_CRUSTA


Color CrustaVisualizer::defaultScopeColor(1,1,1,1);
Color CrustaVisualizer::defaultSectionColor(0.7, 0.8, 1.0, 1.0);
Color CrustaVisualizer::defaultTriangleColor(0.7, 0.6, 0.9, 1.0);
Color CrustaVisualizer::defaultRayColor(0.3, 0.8, 0.4, 1.0);
Color CrustaVisualizer::defaultHitColor(0.9, 0.3, 0.2, 1.0);

Color CrustaVisualizer::defaultSideInColor(0.4, 0.7, 0.8, 1.0);


void CrustaVisualizer::
addScope(const Scope& s, int temp, const Color& color)
{
    Primitive& newPrim = vis->getNewPrimitive(temp);

    newPrim.mode  = GL_LINE_STRIP;
    newPrim.color = color;

    std::vector<Geometry::Point<double,3> > verts;
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
    std::vector<Geometry::Point<double,3> > verts;
    verts.resize(3);
    verts[0] = Geometry::Point<double,3>(0);
    verts[1] = Geometry::Point<double,3>(x*s.getStart()[0], x*s.getStart()[1], x*s.getStart()[2]);
    verts[2] = Geometry::Point<double,3>(x*s.getEnd()[0], x*s.getEnd()[1], x*s.getEnd()[2]);

    newPrim.setVertices(verts);
}


void CrustaVisualizer::
addTriangle(const Triangle& t, int temp, const Color& color)
{
    Primitive& newPrim = vis->getNewPrimitive(temp);

    newPrim.mode  = GL_TRIANGLES;
    newPrim.color = color;

    std::vector<Geometry::Point<double,3> > verts;
    verts.resize(3);
    verts[0] = t.getVert0();
    verts[1] = verts[0] + t.getEdge1();
    verts[2] = verts[0] + t.getEdge2();

    newPrim.setVertices(verts);
}

void CrustaVisualizer::
addRay(const Geometry::Ray<double,3>& r, int temp, const Color& color)
{
    Primitive& newPrim = vis->getNewPrimitive(temp);

    newPrim.mode  = GL_LINES;
    newPrim.color = color;

    std::vector<Geometry::Point<double,3> > verts;
    verts.resize(2);
    verts[0] = r.getOrigin();
    verts[1] = r(0.00001 * 6371000); //hardcoded earth radius

    newPrim.setVertices(verts);
}

void CrustaVisualizer::
addHit(const Geometry::Ray<double,3>& r, const Geometry::HitResult<double>& h, int temp, const Color& color)
{
    if (!h.isValid())
        return;

    Primitive& newPrim = vis->getNewPrimitive(temp);

    newPrim.mode  = GL_POINTS;
    newPrim.color = color;

    std::vector<Geometry::Point<double,3> > verts;
    verts.resize(1);
    verts[0] = r(h.getParameter());

    newPrim.setVertices(verts);
}


void CrustaVisualizer::
addSideIn(const int sideIn, const Scope& s, int temp, const Color& color)
{
    const Geometry::Point<double,3>* corners[4][2] = {
        {&s.corners[3], &s.corners[2]},
        {&s.corners[2], &s.corners[0]},
        {&s.corners[0], &s.corners[1]},
        {&s.corners[1], &s.corners[3]}};

    if (sideIn!=-1)
    {
        std::vector<Geometry::Point<double,3> > verts;
        verts.resize(2);
        verts[0] = *(corners[sideIn][0]);
        verts[1] = *(corners[sideIn][1]);
        addPrimitive(GL_LINES, verts, temp, color);
    }
}


void CrustaVisualizer::
resetNavigationCallback(Misc::CallbackData* cbData)
{
    /* Reset the Vrui navigation transformation: (hardcoded earth radius) */
    Vrui::setNavigationTransformation(Vrui::Point(0,0,0), 1.5*6371000);
}


END_CRUSTA
