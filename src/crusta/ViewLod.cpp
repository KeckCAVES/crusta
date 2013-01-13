#include <crusta/ViewLod.h>

#include <algorithm>

#include <crusta/QuadNodeData.h>
#include <crusta/CrustaSettings.h>

BEGIN_CRUSTA

float ViewLod::
compute(const NodeData& node)
{
    float lod;
    if (!SETTINGS->sliceToolEnable) {
        lod = frustum.calcProjectedRadius(node.boundingCenter,
                                          node.boundingRadius);
    } else {
        lod = std::max(frustum.calcProjectedRadius(node.getEffectiveBoundingCenter(), node.boundingRadius),
                       frustum.calcProjectedRadius(node.boundingCenter, node.boundingRadius));
    }
    lod /= (TILE_RESOLUTION * 0.55f);
    lod = log(lod);
    return lod;
}

END_CRUSTA
