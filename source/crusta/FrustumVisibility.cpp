#include <crusta/FrustumVisibility.h>

#include <algorithm>

#include <crusta/Scope.h>

BEGIN_CRUSTA

bool intersectRaySphere(Scope::Vertex ro, Scope::Vertex rd,
                        Scope::Vertex so, Scope::Scalar sr2)
{
    Scope::Vertex dst;
    dst[0] = ro[0] - so[0];
    dst[1] = ro[1] - so[1];
    dst[2] = ro[2] - so[2];

	Scope::Scalar B = dst[0]*rd[0]  + dst[1]*rd[1]  + dst[2]*rd[2];
	Scope::Scalar C = dst[0]*dst[0] + dst[1]*dst[1] + dst[2]*dst[2] - sr2;
	Scope::Scalar D = B*B - C;

    return D > Scope::Scalar(0);
//	return D > 0 ? -B - sqrt(D) : std::numeric_limits<float>::infinity();
}

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

    if (!frustum.doesSphereIntersect(center, radius))
        return false;

    return true;
}

END_CRUSTA
