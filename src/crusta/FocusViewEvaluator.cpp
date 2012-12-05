#include <crusta/FocusViewEvaluator.h>

#include <algorithm>

#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Vrui.h>

#include <crusta/QuadNodeData.h>

BEGIN_CRUSTA

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

#ifndef CRUSTA_SLICING
    float lod = frustum.calcProjectedRadius(node.boundingCenter,
                                            node.boundingRadius);
#else /* CRUSTA_SLICING */
    float lod = std::max(frustum.calcProjectedRadius(node.getEffectiveBoundingCenter(), node.boundingRadius),
                         frustum.calcProjectedRadius(node.boundingCenter, node.boundingRadius));
#endif /* CRUSTA_SLICING */
    if (lod < 0)
        lod = Math::Constants<float>::max;
    else
    {
#ifndef CRUSTA_SLICING
        double dist = Geometry::dist(node.boundingCenter, focusCenter);
#else /* CRUSTA_SLICING */
        double dist = std::min(Geometry::dist(node.getEffectiveBoundingCenter(), focusCenter),
                               Geometry::dist(node.boundingCenter, focusCenter));
#endif /* CRUSTA_SLICING */
        dist       -= node.boundingRadius;
        if (dist > focusRadius)
            lod /= pow(dist/focusRadius, weight);
    }

    lod /= (TILE_RESOLUTION * 0.55f);
    lod = log(lod);

    return lod;
}

END_CRUSTA
