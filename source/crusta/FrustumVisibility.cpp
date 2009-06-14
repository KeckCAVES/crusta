#include <crusta/FrustumVisibility.h>

#include <crusta/QuadNodeData.h>

BEGIN_CRUSTA

bool FrustumVisibility::
evaluate(const QuadNodeMainData& mainData)
{
///\todo fix-me exaggerated the bounding radius due to instable frustum
    if (!frustum.doesSphereIntersect(mainData.boundingCenter,
                                     2.0f*mainData.boundingRadius))
    {
        return false;
    }

    return true;
}

END_CRUSTA
