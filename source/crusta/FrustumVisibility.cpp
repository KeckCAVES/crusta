#include <crusta/FrustumVisibility.h>

#include <crusta/QuadNodeData.h>
#include <crusta/SliceTool.h>

BEGIN_CRUSTA

bool FrustumVisibility::
evaluate(const NodeData& node)
{
    if (!frustum.doesSphereIntersect(node.boundingCenter,
                                     node.boundingRadius) &&
        !frustum.doesSphereIntersect(node.boundingCenter + SliceTool::getParameters().getShiftVector(),
                                     node.boundingRadius))
    {
        return false;
    }

    return true;
}

END_CRUSTA
