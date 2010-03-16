#include <crusta/FrustumVisibility.h>

#include <crusta/QuadNodeData.h>

BEGIN_CRUSTA

bool FrustumVisibility::
evaluate(const QuadNodeMainData& mainData)
{
    if (!frustum.doesSphereIntersect(mainData.boundingCenter,
                                     mainData.boundingRadius))
    {
        return false;
    }

    return true;
}

END_CRUSTA
