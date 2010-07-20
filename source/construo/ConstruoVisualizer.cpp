#include <construo/ConstruoVisualizer.h>

#include <construo/Converters.h>
#include <construo/SphereCoverage.h>


BEGIN_CRUSTA


void ConstruoVisualizer::
addSphereCoverage(const SphereCoverage& coverage, int temp, const Color& color)
{
    Primitive& newPrim = vis->getNewPrimitive(temp);

    newPrim.mode  = GL_LINE_LOOP;
    newPrim.color = color;

    const SphereCoverage::Points& scv = coverage.getVertices();
    int numVertices = static_cast<int>(scv.size());

    Point3s verts(numVertices);
    for (int i=0; i<numVertices; ++i)
    {
        verts[i][0] = scv[i][0];
        verts[i][1] = Scalar(0);
        verts[i][2] = scv[i][1];
    }

    newPrim.setVertices(verts);
}

void ConstruoVisualizer::
addScopeRefinement(int resolution, Scope::Scalar* s, int temp,
                   const Color& color)
{
    Primitive& newPrim = vis->getNewPrimitive(temp);

    newPrim.mode  = GL_POINTS;
    newPrim.color = color;

    Point3s verts;
    int numSamples = resolution*resolution;
    verts.reserve(numSamples);

    uint numScopeSamples = numSamples*3;
    for (const Scope::Scalar* in=s; in<s+numScopeSamples; in+=3)
    {
        verts.push_back(Point3(in[0], in[1], in[2]));
    }

    newPrim.setVertices(verts);
}


END_CRUSTA
