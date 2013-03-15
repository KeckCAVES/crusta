#include <crusta/FrustumVisibility.h>

#include <crusta/QuadNodeData.h>
#include <crusta/SliceTool.h>
#include <crusta/CrustaSettings.h>

BEGIN_CRUSTA

bool FrustumVisibility::
evaluate(const NodeData& node)
{
    if (!SETTINGS->sliceToolEnable)
    {
        if (!frustum.doesSphereIntersect(node.boundingCenter,
                                         node.boundingRadius))
        {
            return false;
        }

        return true;
    }
    else
    {
        // FIXME: Sorry for this hack - rolf
        if (frustum.doesSphereIntersect(node.getEffectiveBoundingCenter(), 2*node.boundingRadius) ||
            frustum.doesSphereIntersect(node.boundingCenter, 2*node.boundingRadius))
            return true;

        /*
        const SliceTool::SliceParameters &params = SliceTool::getParameters();
        for (size_t i=0; i < params.faultPlanes.size(); ++i) {
            Geometry::Vector<double,3> shiftVector(params.getShiftVector(params.faultPlanes[i]));
            if (frustum.doesSphereIntersect(node.boundingCenter + shiftVector, node.getEffectiveBoundingRadius()))
                return true;
        }
        */

        return false;
    }
}

END_CRUSTA
