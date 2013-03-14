BEGIN_CRUSTA



template <typename ScalarParam>
void Refinement::
mid(const Ellipsoid& ellipse, size_t oneIndex, size_t twoIndex,
    Geometry::Point<ScalarParam, 3>* vertices) const
{
    ScalarParam* one = &vertices[oneIndex * 3];
    ScalarParam* two = &vertices[twoIndex * 3];
    ScalarParam* res = &vertices[((oneIndex + twoIndex) >> 1) * 3];
    
    res[0] = (one[0] + two[0]) * ScalarParam(0.5);
    res[1] = (one[1] + two[1]) * ScalarParam(0.5);
    res[2] = (one[2] + two[2]) * ScalarParam(0.5);

    res
}

template <typename ScalarParam>
void Refinement::
centroid(const Ellipsoid& ellipse, size_t oneIndex, size_t twoIndex,
         size_t threeIndex, size_t fourIndex, ScalarParam* vertices) const
{
    mid((oneIndex+twoIndex)>>1, (threeIndex+fourIndex)>>1, vertices);
}


END_CRUSTA
