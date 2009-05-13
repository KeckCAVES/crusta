#include <crusta/FrustumVisibility.h>

#include <algorithm>

#include <crusta/Scope.h>

BEGIN_CRUSTA

bool FrustumVisibility::
evaluate(const Scope& scope)
{
    Scope::Vertex center;
    for (uint i=0; i<3; ++i)
    {
        float accumulated = 0;
        for (uint j=0; j<4; ++j)
            accumulated += scope.corners[j][i];
        center[i] = accumulated * 0.25f;
    }

    float radius = 0.0f;
    for (uint i=0; i<4; ++i)
    {
        Geometry::Vector<Scope::Scalar, 3> toCorner = scope.corners[i] - center;
        float length = toCorner.mag();
        radius = std::max(radius, length);
    }
    
    return frustum.doesSphereIntersect(center, radius);
}

END_CRUSTA
