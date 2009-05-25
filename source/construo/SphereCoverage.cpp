///\todo fix FRAK'ing cmake !@#!@
#define CONSTRUO_BUILD 1

#include <construo/SphereCoverage.h>

#include <construo/Converters.h>
#include <construo/ImageCoverage.h>
#include <construo/ImageTransform.h>

BEGIN_CRUSTA

static void shortestArc(const Point& ancor,
                        Point& vertex)
{
    static const Point::Scalar twopi = 2.0 * Math::Constants<Point::Scalar>::pi;
    for (uint i=0; i<2; ++i)
    {
        Point::Scalar dist = vertex[i] - ancor[i];
        vertex[i] = dist > Math::Constants<Point::Scalar>::pi ?
                    vertex[i]-twopi : vertex[i];
        vertex[i] = dist < -Math::Constants<Point::Scalar>::pi ?
                    vertex[i]+twopi : vertex[i];
    }
}

static SphereCoverage::Vector
shortestArcShift(const Point& c0, const Point& c1)
{
    SphereCoverage::Vector vec;
    static const Point::Scalar twopi = 2.0 * Math::Constants<Point::Scalar>::pi;
    for (uint i=0; i<2; ++i)
    {
        Point::Scalar dist = c1[0] - c0[0];
        vec[i] = dist> Math::Constants<Point::Scalar>::pi ? -twopi : 0;
        vec[i] = dist<-Math::Constants<Point::Scalar>::pi ?  twopi : 0;
    }

    return vec;
}

SphereCoverage::
SphereCoverage() :
    box(Box::empty)
{}

const SphereCoverage::Points& SphereCoverage::
getVertices() const
{
    return vertices;
}

const Box& SphereCoverage::
getBoundingBox() const
{
    return box;
}

const Point& SphereCoverage::
getCentroid() const
{
    return centroid;
}

bool SphereCoverage::
contains(const Point& v) const
{
    //check bounding box first
    if (!box.contains(v))
        return false;

    //check the point for region changes against all polygon edges:
    int numRegionChanges = 0;
    uint numVertices = static_cast<uint>(vertices.size());
    const Point* prev = &vertices[numVertices-1];
    for (uint i=0; i<numVertices; ++i)
    {
        const Point* curr = &vertices[i];
        if ( ((*prev)[1] <= v[1]) && ((*curr)[1] > v[1]) )
        {
            if ( (v[0]-(*prev)[0]) * ((*curr)[1]-(*prev)[1]) <
                 (v[1]-(*prev)[1]) * ((*curr)[0]-(*prev)[0]) )
            {
                ++numRegionChanges;
            }
        }
        else if ( ((*prev)[1] > v[1]) && ((*curr)[1] <= v[1]) )
        {
            if ( (v[0]-(*curr)[0]) * ((*prev)[1]-(*curr)[1]) <
                 (v[1]-(*curr)[1]) * ((*prev)[0]-(*curr)[0]) )
            {
                ++numRegionChanges;
            }
        }
        prev = curr;
    }
    
    return (numRegionChanges&0x1) != 0x0;
}

uint SphereCoverage::
overlaps(const Box& bBox) const
{
    //check box against the bounding box first
    if (!box.overlaps(bBox))
        return SEPARATE;

    /* check if any polygon edges intersect the box, and check the box's min
       vertex for region changes against all polygon edges: */
    int numRegionChanges = 0;
    uint numVertices = static_cast<uint>(vertices.size());
    const Point* prev = &vertices[numVertices-1];
    for (uint vertexIndex=0; vertexIndex<numVertices; ++vertexIndex)
    {
        const Point* curr = &vertices[vertexIndex];
        
        //check if the current edge intersects the box or is contained by it:
        Point::Scalar lambda0 = Point::Scalar(0);
        Point::Scalar lambda1 = Point::Scalar(1);
        for (int i=0; i<2; ++i)
        {
            //check against min box edge:
            if ((*prev)[i] < bBox.min[i])
            {
                if ((*curr)[i] >= bBox.min[i])
                {
                    Point::Scalar lambda = (bBox.min[i]-(*prev)[i]) /
                                            ((*curr)[i]-(*prev)[i]);
                    if (lambda0 < lambda)
                        lambda0 = lambda;
                }
                else
                {
                    lambda0 = Point::Scalar(1);
                }
            }
            else if ((*curr)[i] < bBox.min[i])
            {
                if ((*prev)[i] >= bBox.min[i])
                {
                    Point::Scalar lambda = (bBox.min[i]-(*prev)[i]) /
                                            ((*curr)[i]-(*prev)[i]);
                    if (lambda1 > lambda)
                        lambda1 = lambda;
                }
                else
                {
                    lambda1 = Point::Scalar(0);
                }
            }
            
            //check against max box edge:
            if ((*prev)[i] > bBox.max[i])
            {
                if ((*curr)[i] <= bBox.max[i])
                {
                    Point::Scalar lambda = (bBox.max[i]-(*prev)[i]) /
                                            ((*curr)[i]-(*prev)[i]);
                    if (lambda0 < lambda)
                        lambda0 = lambda;
                }
                else
                {
                    lambda0 = Point::Scalar(1);
                }
            }
            else if ((*curr)[i] > bBox.max[i])
            {
                if ((*prev)[i] <= bBox.max[i])
                {
                    Point::Scalar lambda = (bBox.max[i]-(*prev)[i]) /
                                            ((*curr)[i]-(*prev)[i]);
                    if(lambda1 > lambda)
                        lambda1 = lambda;
                }
                else
                {
                    lambda1 = Point::Scalar(0);
                }
            }
        }
        
        //box overlaps if current edge is contained by or intersects the box
        if (lambda0 < lambda1)
            return OVERLAPS;
        
        //check the box's min vertex for region changes against the current edge
        if ( ((*prev)[1] <= bBox.min[1])  &&  ((*curr)[1]>bBox.min[1]) )
        {
            if ( (bBox.min[0]-(*prev)[0]) * ((*curr)[1]-(*prev)[1]) <
                 (bBox.min[1]-(*prev)[1]) * ((*curr)[0]-(*prev)[0]) )
            {
                ++numRegionChanges;
            }
        }
        else if ( ((*prev)[1] > bBox.min[1])  &&  ((*curr)[1] <= bBox.min[1]) )
        {
            if( (bBox.min[0]-(*curr)[0]) * ((*prev)[1]-(*curr)[1]) <
                (bBox.min[1]-(*curr)[1]) * ((*prev)[0]-(*curr)[0]) )
            {
                ++numRegionChanges;
            }
        }
        prev = curr;
    }
    
    //box overlaps if its min vertex is contained in the polygon:
    return (numRegionChanges&0x1)!=0x0 ? CONTAINS : SEPARATE;
}

uint SphereCoverage::
overlaps(const SphereCoverage& coverage) const
{
    //account for periodicity. Make sure the centroids are on the shortest arc
    Vector shiftVector = shortestArcShift(centroid, coverage.centroid);
    SphereCoverage test = coverage;
    test.shift(shiftVector);

    //check box against the bounding box first
    if (!box.overlaps(test.box))
        return SEPARATE;

    /* check the points of 'test' for containment. There are 4 possibilities:
       1. all the 'test' points are inside the coverage           -> OVERLAPS
       1.1 all the coverage points are outside 'test'             -> CONTAINS
       2. some are inside others outside the coverage             -> OVERLAPS
       3. all outside and 'test' contains a point of the coverage -> OVERLAPS
       3.1 all the vertices of coverage are outside of 'test'     -> ISCONTAINED
       3.2 all outside and no coverage vertex is in 'test'        -> SEPARATE */
    Points::const_iterator tIt = test.vertices.begin();
    bool testContained = contains(*tIt);
    for (++tIt; tIt!=test.vertices.end(); ++tIt)
    {
        bool isContained = contains(*tIt);
        if (isContained != testContained)
            return OVERLAPS;
    }

    Points::const_iterator cIt = vertices.begin();
    bool coverageContained = test.contains(*cIt);
    for (++cIt; cIt!=vertices.end(); ++cIt)
    {
        bool isContained = test.contains(*cIt);
        if (isContained != coverageContained)
            return OVERLAPS;
    }

    if (testContained)
        return CONTAINS;
    else
        return coverageContained ? ISCONTAINED : SEPARATE;
}

void SphereCoverage::
shift(const Vector& vec)
{
    //go through all the vertices and shift them
    for (Points::iterator it=vertices.begin(); it!=vertices.end(); ++it)
    {
        (*it)[0] += vec[0];
        (*it)[1] += vec[1];
    }
    box.shift(vec);
}


StaticSphereCoverage::
StaticSphereCoverage(uint subdivisions, const Scope& scope)
{
    uint numSamples = pow(2, subdivisions) + 1;
    uint lastSample = numSamples - 1;
    Scope::Vertex* samples = new Scope::Vertex[numSamples];
    uint numVertices = 4 * numSamples - 4;
    vertices.reserve(numVertices);

    /* go through all the side-segments of the scope in order and produce
       samples on each segment through 'subdivisions' number of subdivisions.
       Each sample corresponsing to a vertex of the coverage */
    Scope::Scalar radius = scope.getRadius();
    static const uint remap[4] = {0, 1, 3, 2};
    for (uint start=0; start<4; ++start)
    {
        uint end            = (start+1) % 4;
        samples[0]          = scope.corners[remap[start]];
        samples[lastSample] = scope.corners[remap[end]];

        //iterate over block sizes to generate each subdivision level
        for (uint blockSize=lastSample; blockSize>1; blockSize>>=1)
        {
            uint halfBlockSize = blockSize>>1;
            //iterate over the blocks of a level to generate the mid points
            for (uint block=0; block<lastSample; block+=blockSize)
            {
                Scope::Vertex& lower = samples[block];
                Scope::Vertex& upper = samples[block+blockSize];
                Scope::Vertex& mid   = samples[block+halfBlockSize];
                //accumulate the norm while computing the mid-point
                double  norm  = 0;
                for (uint i=0; i<3; ++i)
                {
                    mid[i] = (lower[i] + upper[i]) * 0.5;
                    norm += mid[i]*mid[i];
                }
                //normalize
                norm = radius / sqrt(norm);
                for (uint i=0; i<3; ++i)
                    mid[i] *= norm;
            }
        }

        //append the new samples as spherical vertices to the set
        for (uint i=0; i<numSamples-1; ++i)
            vertices.push_back(Converter::cartesianToSpherical(samples[i]));
    }

    //clean-up temporary memory used to store the samples
    delete[] samples;

    //take care of periodicity of spherical space
    for (uint i=1; i<numVertices; ++i)
        shortestArc(vertices[i-1], vertices[i]);
///\todo this is bad fix for the exact pole points of the triacontahedron
#if 0
    for (uint i=0; i<numVertices; ++i)
    {
        if (vertices[i][0]==0 && (vertices[i][1]==0 ||
             (Math::Constants<Point::Scalar>::pi - vertices[i][1])<1e-5))
        {
            Point& prev = vertices[(i-1)%numVertices];
            Point& next = vertices[(i+1)%numVertices];
            vertices[i][0] = (prev[0] + next[0]) * 0.5;
        }
    }
#endif

    //compute box and centroid
    centroid = Point(0,0);
    for (uint i=0; i<numVertices; ++i)
    {
        Point& cur = vertices[i];
        box.addPoint(cur);
        centroid[0] += cur[0];
        centroid[1] += cur[1];
    }

    Point::Scalar norm = Point::Scalar(1) / Point::Scalar(numVertices);
    centroid[0] *= norm;
    centroid[1] *= norm;
}

StaticSphereCoverage::
StaticSphereCoverage(uint subdivisions, const ImageCoverage* coverage,
                     const ImageTransform* transform)
{
    assert(coverage!=NULL && transform!=NULL);

    uint numSamples     = pow(2, subdivisions) + 1;
    double sampleStep   = 1.0/(numSamples-1);
    uint numImgVertices = coverage->getNumVertices();
    uint numVertices    = numImgVertices*numSamples - numImgVertices;
    vertices.reserve(numVertices);
    
    /* go through all the side-segments of the image in order and produce
     samples on each segment through 'subdivisions' number of subdivisions.
     Each sample corresponsing to a vertex of the coverage */
    for (uint start=0; start<numImgVertices; ++start)
    {
        uint end           = (start+1) % numImgVertices;
        const Point& lower = coverage->getVertex(start);
        const Point& upper = coverage->getVertex(end);

        //generate all the samples along that segment
        double alpha = 0.0;
        for (uint i=0; i<numSamples-1; ++i, alpha+=sampleStep)
        {
            Point imgPoint((1.0-alpha)*lower[0] + alpha*upper[0],
                           (1.0-alpha)*lower[1] + alpha*upper[1]);
            Point newVertex = transform->imageToWorld(imgPoint);
            vertices.push_back(newVertex);
        }
    }
    
    //take care of periodicity of spherical space
    centroid = Point(0,0);
    box.addPoint(vertices[0]);
    centroid[0] += vertices[0][0];
    centroid[1] += vertices[0][1];
    for (uint i=1; i<numVertices; ++i)
    {
        Point& prev = vertices[i-1];
        Point& cur  = vertices[i];
        
        shortestArc(prev, cur);
        box.addPoint(cur);
        centroid[0] += cur[0];
        centroid[1] += cur[1];
    }
    
    Point::Scalar norm = Point::Scalar(1) / Point::Scalar(numVertices);
    centroid[0] *= norm;
    centroid[1] *= norm;
}


END_CRUSTA
