#include <crusta/FrustumVisibility.h>

#include <crusta/QuadNodeData.h>
#ifdef CRUSTA_SLICING
#include <crusta/SliceTool.h>
#endif /* CRUSTA_SLICING */

BEGIN_CRUSTA

bool FrustumVisibility::
evaluate(const NodeData& node)
{
#ifndef CRUSTA_SLICING
    if (!frustum.doesSphereIntersect(node.boundingCenter,
                                     node.boundingRadius))
    {
        return false;
    }

    return true;
#else /* CRUSTA_SLICING */
    // FIXME: Sorry for this hack - rolf
    if (frustum.doesSphereIntersect(node.getEffectiveBoundingCenter(), 2*node.boundingRadius) ||
        frustum.doesSphereIntersect(node.boundingCenter, 2*node.boundingRadius))
        return true;

    /*
    const SliceTool::SliceParameters &params = SliceTool::getParameters();

    for (size_t i=0; i < params.faultPlanes.size(); ++i) {
        Vector3 shiftVector(params.getShiftVector(params.faultPlanes[i]));
        if (frustum.doesSphereIntersect(node.boundingCenter + shiftVector, node.getEffectiveBoundingRadius()))
            return true;
    }
    */

    return false;
#endif /* CRUSTA_SLICING */
}

END_CRUSTA
