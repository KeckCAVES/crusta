#include <crusta/SurfaceApproximation.h>


namespace crusta {


void SurfaceApproximation::
clear()
{
    nodes.clear();;
    visibles.clear();;
    //neighbors.clear();
}

void SurfaceApproximation::
add(const NodeMainData& node, bool isVisible)
{
    nodes.push_back(node);
    if (isVisible)
        visibles.push_back(nodes.size()-1);
}

NodeMainData& SurfaceApproximation::
visible(size_t index)
{
    assert(index<visibles.size());
    return nodes[visibles[index]];
}

const NodeMainData& SurfaceApproximation::
visible(size_t index) const
{
    assert(index<visibles.size());
    return nodes[visibles[index]];
}

void SurfaceApproximation::
processNeighborhood()
{
///\todo Rolf, please add your neighbor processing here
}


} //namespace crusta
