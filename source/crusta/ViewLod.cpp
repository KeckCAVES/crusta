#include <crusta/ViewLod.h>

#include <algorithm>

#include <crusta/QuadNodeData.h>

BEGIN_CRUSTA

float ViewLod::
compute(const QuadNodeMainData& mainData)
{
    float lod = frustum.calcProjectedRadius(mainData.boundingCenter,
                                            mainData.boundingRadius);
    lod /= (TILE_RESOLUTION * 0.55f);
    lod = log(lod);
    return lod;
}

END_CRUSTA
