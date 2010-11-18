#include <crusta/FrustumVisibility.h>

#include <crusta/QuadNodeData.h>

BEGIN_CRUSTA

bool FrustumVisibility::
evaluate(const NodeData& node)
{
    if (!frustum.doesSphereIntersect(node.boundingCenter,
                                     node.boundingRadius))
    {
        return false;
    }

    return true;
}

END_CRUSTA
