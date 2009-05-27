#include <crusta/FrustumVisibility.h>

#include <crusta/Node.h>

BEGIN_CRUSTA

/**\todo cull everything that is behind the sphere using ray casting on the
bounding box of the data */
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
evaluate(const Node* node)
{
    if (!frustum.doesSphereIntersect(node->boundingCenter,node->boundingRadius))
        return false;

    return true;
}

END_CRUSTA
