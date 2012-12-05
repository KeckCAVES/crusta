#ifndef _SurfaceApproximation_H_
#define _SurfaceApproximation_H_


#include <crusta/QuadNodeDataBundles.h>


BEGIN_CRUSTA


/** stores all the elements needed to reconstruct and display an approximation
    of a globe surface */
struct SurfaceApproximation
{
    typedef std::vector<int>       Indices;
    // DISABLED as not used, and array should be replaced with struct
    //typedef int                    Neighbors[4];
    //typedef std::vector<Neighbors> NeighborIndices;

    /** clear the surface approximation */
    void clear();

    /** add a node to the representation as contributing to the display or
        not */
    void add(const NodeMainData& node, bool isVisible);
    /** returns the data of the index'th visible node */
    NodeMainData& visible(size_t index);
    const NodeMainData& visible(size_t index) const;
/**\todo Rolf, please add a meaningful convenience function for accessing
neighbors*/

    /** generate neighborhood information for the visible part of the
        representation */
    void processNeighborhood();

    /** stores the nodes making up a gap- and overlap free tiling of the global
        surface defined by a LOD scheme */
    NodeMainDatas nodes;
    /** stores indices to nodes of the surface representation that are part of
        the visibile subset */
    Indices visibles;
    /** stores sets of indices to neighbors nodes in the surface representation
        for the visible subset */
    // DISABLED as not yet used anywhere
    //NeighborIndices neighbors;
};


END_CRUSTA

#endif //_SurfaceApproximation_H_
