BEGIN_CRUSTA

template <typename ScalarParam>
void Scope::
mid(uint oneIndex, uint twoIndex, ScalarParam* vertices,
    ScalarParam radius) const
{
    ScalarParam* one = &vertices[oneIndex * 3];
    ScalarParam* two = &vertices[twoIndex * 3];
    ScalarParam* res = &vertices[((oneIndex + twoIndex) >> 1) * 3];
    
    res[0] = (one[0] + two[0]) * ScalarParam(0.5);
    res[1] = (one[1] + two[1]) * ScalarParam(0.5);
    res[2] = (one[2] + two[2]) * ScalarParam(0.5);
    
    ScalarParam invLen = radius /
                         sqrt(res[0]*res[0] + res[1]*res[1] + res[2]*res[2]);
    res[0] *= invLen;
    res[1] *= invLen;
    res[2] *= invLen;
}

template <typename ScalarParam>
void Scope::
centroid(uint oneIndex, uint twoIndex, uint threeIndex, uint fourIndex,
         ScalarParam* vertices, ScalarParam radius) const
{
    mid((oneIndex+twoIndex)>>1, (threeIndex+fourIndex)>>1, vertices, radius);
}

template <typename ScalarParam>
void Scope::
getRefinement(uint resolution, ScalarParam* vertices) const
{
    //set the initial four corners from the scope
    uint cornerIndices[] = {
        0, (resolution-1)*3, (resolution-1)*resolution*3,
        ((resolution-1)*resolution + (resolution-1))*3
    };
    for (uint i=0; i<4; ++i)
    {
        uint ci = cornerIndices[i];
        for (uint j=0; j<3; ++j)
            vertices[ci+j] = corners[i][j];
    }
    
    //refine the rest
    ScalarParam radius = static_cast<ScalarParam>(getRadius());
    uint endIndex  = resolution * resolution;
    for (uint blockSize=resolution-1; blockSize>1; blockSize>>=1)
    {
        uint blockSizeY = blockSize * resolution;
        for (uint startY=0, endY=blockSizeY; endY<endIndex;
             startY+=blockSizeY, endY+=blockSizeY)
        {
            for (uint startX=0, endX=blockSize; endX<resolution;
                 startX+=blockSize, endX+=blockSize)
            {
                /* top mid point (only if on the edge where it has not already
                   been computed by the neighbor) */
                if (startY == 0)
                    mid(endX, startX, vertices, radius);
                /* left mid point (only if on the edge where it has not already
                   been computed by the neighbor */
                if (startX == 0)
                    mid(startY, endY, vertices, radius);
                //bottom mid point
                mid(endY+startX, endY+endX, vertices, radius);
                //right mid point
                mid(endY+endX, startY+endX, vertices, radius);
                //centroid
                centroid(startY+startX, endY+startX, endY+endX, startY+endX,
                         vertices, radius);
            }
        }
    }
}

END_CRUSTA
