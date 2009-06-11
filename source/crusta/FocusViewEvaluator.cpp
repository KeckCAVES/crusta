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
compute(const QuadNodeMainData& mainData)
{
    double weight = 2.0;

    float lod = frustum.calcProjectedRadius(mainData.boundingCenter,
                                            mainData.boundingRadius);
    if (lod < 0)
        lod = Math::Constants<float>::max;
    else
    {
        double dist = Geometry::dist(mainData.boundingCenter, focusCenter);
        dist       -= mainData.boundingRadius;
        if (dist > focusRadius)
            lod /= pow(dist/focusRadius, weight);
    }

    lod /= (TILE_RESOLUTION * 0.55f);
    lod = log(lod);
    
    return lod;
}

END_CRUSTA
