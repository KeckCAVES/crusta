#include <crusta/FocusViewEvaluator.h>

#include <algorithm>

#include <crusta/Node.h>

BEGIN_CRUSTA

float FocusViewEvaluator::
compute(const Node* node)
{
///\todo implement focus part
    float lod = frustum.calcProjectedRadius(node->boundingCenter,
                                            node->boundingRadius);
    if (lod < 0)
        lod = Math::Constants<float>::max;
    lod /= (TILE_RESOLUTION * 0.55f);
    lod = log(lod);
    return lod;
}

END_CRUSTA
