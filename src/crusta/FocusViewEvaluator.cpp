#include <crusta/FocusViewEvaluator.h>

#include <algorithm>

#include <crusta/QuadNodeData.h>
#include <crusta/CrustaSettings.h>

#include <crusta/vrui.h>

namespace crusta {

void FocusViewEvaluator::
setFocusFromDisplay()
{
	Vrui::NavTransform invXform = Vrui::getInverseNavigationTransformation();
	focusCenter = Geometry::Point<double, 3>(
		invXform.transform(Vrui::getDisplayCenter()));
	focusRadius = invXform.getScaling() * Vrui::getDisplaySize() * 0.5;
}

float FocusViewEvaluator::
compute(const NodeData& node)
{
    double weight = 2.0;

    float lod;
    if (!SETTINGS->sliceToolEnable) {
        lod = frustum.calcProjectedRadius(node.boundingCenter,
                                          node.boundingRadius);
    } else {
        lod = std::max(frustum.calcProjectedRadius(node.getEffectiveBoundingCenter(), node.boundingRadius),
                       frustum.calcProjectedRadius(node.boundingCenter, node.boundingRadius));
    }
    if (lod < 0)
        lod = Math::Constants<float>::max;
    else
    {
        double dist;
        if (!SETTINGS->sliceToolEnable) {
            dist = Geometry::dist(node.boundingCenter, focusCenter);
        } else {
            dist = std::min(Geometry::dist(node.getEffectiveBoundingCenter(), focusCenter),
                            Geometry::dist(node.boundingCenter, focusCenter));
        }
        dist -= node.boundingRadius;
        if (dist > focusRadius)
            lod /= pow(dist/focusRadius, weight);
    }

    lod /= (TILE_RESOLUTION * 0.55f);
    lod = log(lod);

    return lod;
}

} //namespace crusta
