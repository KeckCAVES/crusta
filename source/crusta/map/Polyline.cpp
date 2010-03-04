#include <crusta/map/Polyline.h>


BEGIN_CRUSTA

const Point3f&  Polyline::
getCentroid()
{
    return centroid;
}

const Point3fs& Polyline::
getRelativeControlPoints()
{
    return relativeControlPoints;
}


void Polyline::
recomputeRelativePoints()
{
    //compute the centroid
    Point3 newCentroid(0);
    int numControlPoints = static_cast<int>(controlPoints.size());
    for (int i=0; i<numControlPoints; ++i)
    {
        for (int j=0; j<3; ++j)
            newCentroid[j] += controlPoints[i][j];
    }
    Scalar invNorm = Scalar(1) / numControlPoints;
    for (int j=0; j<3; ++j)
        centroid[j] = newCentroid[j] * invNorm;

    //compute the relative control points
    relativeControlPoints.resize(numControlPoints);
    for (int i=0; i<numControlPoints; ++i)
    {
        for (int j=0; j<3; ++j)
            relativeControlPoints[i][j] = controlPoints[i][j] - centroid[j];
    }
}

Shape::Id Polyline::
addControlPoint(const Point3& pos, End end)
{
    Shape::Id ret = Shape::addControlPoint(pos, end);
    recomputeRelativePoints();
    return ret;
}

bool Polyline::
moveControlPoint(const Id& id, const Point3& pos)
{
    bool ret = Shape::moveControlPoint(id, pos);
    recomputeRelativePoints();
    return ret;
}

void Polyline::
removeControlPoint(const Id& id)
{
    Shape::removeControlPoint(id);
    recomputeRelativePoints();
}


END_CRUSTA
