#include <FrustumVisibility.h>

#include <Scope.h>

#include <algorithm>

BEGIN_CRUSTA

bool FrustumVisibility::
evaluate(const Scope& scope)
{
    Point center;
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
        Vector toCorner = scope.corners[i] - center;
        float length = toCorner.mag();
        radius = std::max(radius, length);
    }
    
    return frustum.containsSphere(center, radius);
}

END_CRUSTA
