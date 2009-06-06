#include <crusta/Node.h>

#include <crusta/Crusta.h>

BEGIN_CRUSTA

Node::
Node() :
    demTile(DemFile::INVALID_TILEINDEX),
    colorTile(ColorFile::INVALID_TILEINDEX), mainBuffer(NULL),
    pinned(false), parent(NULL), children(NULL)
{}

void Node::
computeBoundingSphere()
{
    assert(mainBuffer != NULL);
    DemHeight* range = mainBuffer->getData().elevationRange;
    DemHeight avgElevation = (range[0] + range[1]) * DemHeight(0.5);
    avgElevation *= Crusta::getVerticalScale();

    boundingCenter = scope.getCentroid(SPHEROID_RADIUS + avgElevation);

    boundingRadius = Scope::Scalar(0);
    for (int i=0; i<4; ++i)
    {
        for (int j=0; j<2; ++j)
        {
            Scope::Vertex corner = scope.corners[i];
            Scope::Scalar norm = Scope::Scalar(SPHEROID_RADIUS);
            norm += range[j]*Crusta::getVerticalScale();
            norm /= sqrt(corner[0]*corner[0] + corner[1]*corner[1] +
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

void Node::
init()
{
    assert(mainBuffer != NULL);

    //compute the centroid on the average elevation (see split)
#if USING_AVERAGE_HEIGHT
    DemHeight* range = mainBuffer->getData().elevationRange;
    DemHeight avgElevation = (range[0] + range[1]) * DemHeight(0.5);
    Scope::Vertex scopeCentroid = scope.getCentroid(SPHEROID_RADIUS +
                                                    avgElevation);
#else
    Scope::Vertex scopeCentroid = scope.getCentroid(SPHEROID_RADIUS);
#endif //USING_AVERAGE_HEIGHT
    centroid[0] = scopeCentroid[0];
    centroid[1] = scopeCentroid[1];
    centroid[2] = scopeCentroid[2];

    computeBoundingSphere();
}


END_CRUSTA
