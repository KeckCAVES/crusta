#include <crusta/ViewLod.h>

#include <algorithm>

#include <crusta/Node.h>

BEGIN_CRUSTA

float ViewLod::
compute(const Node* node)
{
    float lod = frustum.calcProjectedRadius(node->boundingCenter,
                                            node->boundingRadius);
    lod /= (TILE_RESOLUTION * 0.55f);
    lod = log(lod);
    return lod;
}

END_CRUSTA
