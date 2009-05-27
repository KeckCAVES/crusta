#include <crusta/Node.h>

BEGIN_CRUSTA

Node::
Node() :
    demTile(DemFile::INVALID_TILEINDEX),
    colorTile(ColorFile::INVALID_TILEINDEX), mainBuffer(NULL),
    videoBuffer(NULL), parent(NULL), children(NULL)
{
    elevationRange[0] = elevationRange[1] = DemHeight(0);
    childDemTiles[0] = childDemTiles[1] = DemFile::INVALID_TILEINDEX;
    childDemTiles[2] = childDemTiles[4] = DemFile::INVALID_TILEINDEX;
    childColorTiles[0] = childColorTiles[1] = ColorFile::INVALID_TILEINDEX;
    childColorTiles[2] = childColorTiles[4] = ColorFile::INVALID_TILEINDEX;
}

void Node::
init(const DemHeight range[2])
{
    elevationRange[0] = range[0];
    elevationRange[1] = range[1];
    //compute the centroid on the average elevation (see split)
    DemHeight avgElevation = (elevationRange[0] + elevationRange[1]) *
                             DemHeight(0.5);
#if USING_AVERAGE_HEIGHT
    Scope::Vertex scopeCentroid = scope.getCentroid(SPHEROID_RADIUS +
                                                    avgElevation);
#else
    Scope::Vertex scopeCentroid = scope.getCentroid(SPHEROID_RADIUS);
#endif //USING_AVERAGE_HEIGHT
    centroid[0] = scopeCentroid[0];
    centroid[1] = scopeCentroid[1];
    centroid[2] = scopeCentroid[2];

///\todo remove with pre-processor fix
    boundingCenter = scope.getCentroid(SPHEROID_RADIUS + avgElevation);

    boundingRadius = Scope::Scalar(0);
    for (int i=0; i<4; ++i)
    {
        for (int j=0; j<2; ++j)
        {
            Scope::Vertex corner = scope.corners[i];
            Scope::Scalar norm =
                Scope::Scalar(SPHEROID_RADIUS+elevationRange[j]) /
                sqrt(corner[0]*corner[0] + corner[1]*corner[1] +
                     corner[2]*corner[2]);
            Scope::Vertex toCorner;
            toCorner[0] = corner[0]*norm - boundingCenter[0];
            toCorner[1] = corner[1]*norm - boundingCenter[1];
            toCorner[2] = corner[2]*norm - boundingCenter[2];
            norm = sqrt(toCorner[0]*toCorner[0] + toCorner[1]*toCorner[1] +
                        toCorner[2]*toCorner[2]);
            boundingRadius = std::max(boundingRadius, norm);
        }
    }
}


END_CRUSTA
