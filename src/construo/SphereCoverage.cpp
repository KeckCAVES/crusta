#include <construo/SphereCoverage.h>

#include <cassert>

#include <construo/Converters.h>
#include <construo/ImageCoverage.h>
#include <construo/ImageTransform.h>

#define DEBUG_COVERAGE 0
#if DEBUG_COVERAGE
#include <cstdio>
#endif //DEBUG_COVERAGE

BEGIN_CRUSTA

static const Point::Scalar EPSILON = 1e-5;
static const Point::Scalar PI      = Math::Constants<Point::Scalar>::pi;
static const Point::Scalar TWOPI   = 2.0 * PI;
static const Point::Scalar HALFPI  = 0.5 * PI;

/** Corrects issues with the representation of a coverage, e.g. the longitude
    value at a pole, or shifts the representation of the vertex by periods to
    find the one that is closest to the anchor in the linear lat/lon space */
static void
fixLatLon(const Point& anchor, Point& vertex)
{
    //assign the previous longitude to a vertex at pole latitude
    if ((Math::abs( HALFPI - vertex[1]) < EPSILON) ||
        (Math::abs(-HALFPI - vertex[1]) < EPSILON))
    {
        vertex[0] = anchor[0];
    }

    //find the representation closest to the anchor
    for (int i=0; i<2; ++i)
    {
        Point::Scalar tmp = vertex[i]>anchor[i] ?
                            vertex[i]-TWOPI : vertex[i]+TWOPI;
        vertex[i] = Math::abs(tmp-anchor[i])<Math::abs(vertex[i]-anchor[i]) ?
                    tmp : vertex[i];
    }
}

static bool
segmentIntersect(Point a, Point b, Point c, Point d)
{
    //translate segments such that 'a' is new origin
    b[0] -= a[0];  b[1] -= a[1];
    c[0] -= a[0];  c[1] -= a[1];
    d[0] -= a[0];  d[1] -= a[1];

    //rotate such that ab oriented toward the positive x axis
    Point::Scalar distab = Geometry::dist(a, b);
    Point::Scalar cosine = b[0] / distab;
    Point::Scalar sine   = b[1] / distab;

    Point::Scalar tmp;
    tmp  = c[0]*cosine + c[1]*sine;
    c[1] = c[1]*cosine - c[0]*sine;
    c[0] = tmp;

    tmp  = d[0]*cosine + d[1]*sine;
    d[1] = d[1]*cosine - d[0]*sine;
    d[0] = tmp;

    //intersection exists if the rotated cd crosses the zero Y
    if ((c[1]<0 && d[1]<0) || (c[1]>0 && d[1]>0))
        return false;

    //check that the intersection is within the range of ab
    Point::Scalar y0 = d[0] + (c[0]-d[0])*d[1]/(d[1]-c[1]);

    if (y0<0 || y0>distab)
        return false;

    return true;
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

bool SphereCoverage::
contains(const Point& v) const
{
    //check bounding box first
    if (!box.contains(v))
        return false;

    //check the point for region changes against all polygon edges:
    int numRegionChanges = 0;
    size_t numVertices = static_cast<size_t>(vertices.size());
    const Point* prev = &vertices[numVertices-1];
    for (size_t i=0; i<numVertices; ++i)
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

size_t SphereCoverage::
overlaps(const Box& bBox) const
{
    //check box against the bounding box first
    if (!box.overlaps(bBox))
        return SEPARATE;

    /* check if any polygon edges intersect the box, and check the box's min
       vertex for region changes against all polygon edges: */
    int numRegionChanges = 0;
    size_t numVertices = static_cast<size_t>(vertices.size());
    const Point* prev = &vertices[numVertices-1];
    for (size_t vertexIndex=0; vertexIndex<numVertices; ++vertexIndex)
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

size_t SphereCoverage::
overlaps(const SphereCoverage& coverage) const
{
    //must check overlap in the 4 spaces produced by periodicity
    size_t res = checkOverlap(coverage);
    if (res != SEPARATE)
        return res;

    Vector shifts[3] = {Vector(TWOPI,0), Vector(0,PI), Vector(TWOPI,PI)};
    for (int i=0; i<3; ++i)
    {
        SphereCoverage cov = coverage;
        cov.shift(shifts[i]);
        res = checkOverlap(cov);
        if (res != SEPARATE)
            break;
    }

    return res;
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


void SphereCoverage::
addVertex(Point& vertex)
{
#if DEBUG_COVERAGE
fprintf(stderr, "(%f, %f)  --  ", vertex[0], vertex[1]);
#endif //DEBUG_COVERAGE
    if (vertices.empty())
    {
#if DEBUG_COVERAGE
fprintf(stderr, "<%f, %f>\n", vertex[0], vertex[1]);
#endif //DEBUG_COVERAGE
        vertices.push_back(vertex);
    }
    else
    {
        const Point& last = vertices.back();
        fixLatLon(last, vertex);
#if DEBUG_COVERAGE
fprintf(stderr, "<%f, %f>\n", vertex[0], vertex[1]);
#endif //DEBUG_COVERAGE
        //when coming from a pole add a pole vertex at the new longitude
        if ((Math::abs( HALFPI - last[1]) < EPSILON) ||
            (Math::abs(-HALFPI - last[1]) < EPSILON))
        {
            vertices.push_back(Point(vertex[0], last[1]));
        }

        vertices.push_back(vertex);
    }
    box.addPoint(vertex);
}

size_t SphereCoverage::
checkOverlap(const SphereCoverage& coverage) const
{
    //check box against the bounding box first
    if (!box.overlaps(coverage.box))
        return SEPARATE;

    const Points& ref  = getVertices();
    int numRef         = static_cast<int>(ref.size());
    const Points& test = coverage.getVertices();
    int numTest        = static_cast<int>(test.size());

    //check for non-containment intersection by check edge crosses
    for (int i=0; i<numRef-1; ++i)
    {
        for (int j=0; j<numTest-1; ++j)
        {
            if (segmentIntersect(ref[i], ref[i+1], test[j], test[j+1]))
                return OVERLAPS;
        }
    }

    //since there is no overlap on the edges, check containment
    bool contained = true;
    for (int i=0; i<numTest; ++i)
    {
        if (!contains(test[i]))
        {
            contained = false;
            break;
        }
    }
    if (contained)
        return CONTAINS;

    contained = true;
    for (int i=0; i<numRef; ++i)
    {
        if (!coverage.contains(ref[i]))
        {
            contained = false;
            break;
        }
    }
    if (contained)
        return ISCONTAINED;

    //if there is no boundary intersection and no containment, they are separate
    return SEPARATE;
}

StaticSphereCoverage::
StaticSphereCoverage(size_t subdivisions, const Scope& scope)
{
    size_t numSamples = pow(2, subdivisions) + 1;
    size_t lastSample = numSamples - 1;
    Scope::Vertex* samples = new Scope::Vertex[numSamples];
    size_t numVertices = 4 * numSamples - 4;
    vertices.reserve(numVertices);

    /* go through all the side-segments of the scope in order and produce
       samples on each segment through 'subdivisions' number of subdivisions.
       Each sample corresponsing to a vertex of the coverage */
    static const size_t remap[4] = {0, 1, 3, 2};
    Scope::Scalar radius       = scope.getRadius();
    for (size_t start=0; start<4; ++start)
    {
        size_t end            = (start+1) % 4;
        samples[0]          = scope.corners[remap[start]];
        samples[lastSample] = scope.corners[remap[end]];

        //iterate over block sizes to generate each subdivision level
        for (size_t blockSize=lastSample; blockSize>1; blockSize>>=1)
        {
            size_t halfBlockSize = blockSize>>1;
            //iterate over the blocks of a level to generate the mid points
            for (size_t block=0; block<lastSample; block+=blockSize)
            {
                Scope::Vertex& lower = samples[block];
                Scope::Vertex& upper = samples[block+blockSize];
                Scope::Vertex& mid   = samples[block+halfBlockSize];
                //accumulate the norm while computing the mid-point
                double  norm  = 0;
                for (size_t i=0; i<3; ++i)
                {
                    mid[i] = (lower[i] + upper[i]) * 0.5;
                    norm += mid[i]*mid[i];
                }
                //normalize
                norm = radius / sqrt(norm);
                for (size_t i=0; i<3; ++i)
                    mid[i] *= norm;
            }
        }

        //append the new samples as spherical vertices to the set
        for (size_t i=0; i<numSamples-1; ++i)
        {
            Point newVertex = Converter::cartesianToSpherical(samples[i]);
            addVertex(newVertex);
        }
    }

    //shift such that the max corner of the box is within (-pi,pi]x(-pi/2,pi/2]
    Vector shiftVec;
    shiftVec[0] = box.max[0]>PI     ? -TWOPI : 0;
    shiftVec[1] = box.max[1]>HALFPI ? -PI    : 0;

#if DEBUG_COVERAGE
fprintf(stderr, "[[%f, %f]]\n", shiftVec[0], shiftVec[1]);
#endif //DEBUG_COVERAGE

    if (shiftVec[0]!=0 || shiftVec[1]!=0)
        shift(shiftVec);

    //clean-up temporary memory used to store the samples
    delete[] samples;
}

StaticSphereCoverage::
StaticSphereCoverage(size_t subdivisions, const ImageCoverage* coverage,
                     const ImageTransform* transform)
{
    assert(coverage!=NULL && transform!=NULL);

    size_t numSamples     = pow(2, subdivisions) + 1;
    double sampleStep   = 1.0/(numSamples-1);
    size_t numImgVertices = coverage->getNumVertices();
    size_t numVertices    = numImgVertices*numSamples - numImgVertices;
    vertices.reserve(numVertices);

    /* go through all the side-segments of the image in order and produce
     samples on each segment through 'subdivisions' number of subdivisions.
     Each sample corresponsing to a vertex of the coverage */
    for (size_t start=0; start<numImgVertices; ++start)
    {
        size_t end           = (start+1) % numImgVertices;
        const Point& lower = coverage->getVertex(start);
        const Point& upper = coverage->getVertex(end);

        //generate all the samples along that segment
        double alpha = 0.0;
        for (size_t i=0; i<numSamples-1; ++i, alpha+=sampleStep)
        {
            Point imgPoint((1.0-alpha)*lower[0] + alpha*upper[0],
                           (1.0-alpha)*lower[1] + alpha*upper[1]);
            Point newVertex = transform->imageToWorld(imgPoint);
            addVertex(newVertex);
        }
    }

///\todo special case handling for global coverages

    //shift such that the max corner of the box is within (-pi,pi]x(-pi/2,pi/2]
    Vector shiftVec;
    shiftVec[0] = box.max[0]>PI     ? -TWOPI : 0;
    shiftVec[1] = box.max[1]>HALFPI ? -PI    : 0;

#if DEBUG_COVERAGE
fprintf(stderr, "[[%f, %f]]\n", shiftVec[0], shiftVec[1]);
#endif //DEBUG_COVERAGE

    if (shiftVec[0]!=0 || shiftVec[1]!=0)
        shift(shiftVec);
}


END_CRUSTA
