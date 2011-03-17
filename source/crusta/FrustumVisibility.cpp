#include <crusta/FrustumVisibility.h>

#include <crusta/QuadNodeData.h>
#include <crusta/SliceTool.h>

BEGIN_CRUSTA

bool FrustumVisibility::
evaluate(const NodeData& node)
{
    if (frustum.doesSphereIntersect(node.boundingCenter, node.boundingRadius))
        return true;

    const SliceTool::SliceParameters &params = SliceTool::getParameters();

    for (size_t i=0; i < params.faultPlanes.size(); ++i) {
        Vector3 shiftVector(params.getShiftVector(params.faultPlanes[i]));
        if (frustum.doesSphereIntersect(node.boundingCenter + shiftVector, node.boundingRadius))
            return true;
    }

    return false;
}

END_CRUSTA
