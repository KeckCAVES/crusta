#include <crusta/QuadNodeData.h>

#include <cassert>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>

BEGIN_CRUSTA

QuadNodeMainData::AgeStampedControlPointHandle::
AgeStampedControlPointHandle() :
    age(~0), handle()
{}

QuadNodeMainData::AgeStampedControlPointHandle::
AgeStampedControlPointHandle(const AgeStamp& iAge,
                             const Shape::ControlPointHandle& iHandle) :
    age(iAge), handle(iHandle)
{}

QuadNodeMainData::
QuadNodeMainData(uint size) :
    lineCoverageDirty(false), lineCoverageAge(0), index(TreeIndex::invalid),
    boundingCenter(0,0,0), boundingRadius(0)
{
    geometry = new Vertex[size*size];
    height   = new DemHeight[size*size];
    color    = new TextureColor[size*size];

    demTile   = DemFile::INVALID_TILEINDEX;
    colorTile = ColorFile::INVALID_TILEINDEX;
    for (int i=0; i<4; ++i)
    {
        childDemTiles[i]       = DemFile::INVALID_TILEINDEX;
        childColorTiles[i]     = ColorFile::INVALID_TILEINDEX;
    }

    centroid[0] = centroid[1] = centroid[2] = DemHeight(0.0);
    elevationRange[0] = elevationRange[1]   = DemHeight(0.0);
}
QuadNodeMainData::
~QuadNodeMainData()
{
    delete[] geometry;
    delete[] height;
    delete[] color;
}

void QuadNodeMainData::
computeBoundingSphere(Scalar verticalScale)
{
    DemHeight avgElevation = (elevationRange[0] + elevationRange[1]);
    avgElevation          *= DemHeight(0.5)* verticalScale;

    boundingCenter = scope.getCentroid(SPHEROID_RADIUS + avgElevation);

    boundingRadius = Scope::Scalar(0);
    for (int i=0; i<4; ++i)
    {
        for (int j=0; j<2; ++j)
        {
            Scope::Vertex corner = scope.corners[i];
            Scope::Scalar norm   = Scope::Scalar(SPHEROID_RADIUS);
            norm += elevationRange[j]*verticalScale;
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

void QuadNodeMainData::
init(Scalar verticalScale)
{
    //compute the centroid on the average elevation (see split)
    Scope::Vertex scopeCentroid = scope.getCentroid(SPHEROID_RADIUS);
    centroid[0] = scopeCentroid[0];
    centroid[1] = scopeCentroid[1];
    centroid[2] = scopeCentroid[2];

    computeBoundingSphere(verticalScale);
}


QuadNodeVideoData::
QuadNodeVideoData(uint size)
{
    createTexture(geometry, GL_RGB32F_ARB, size);
    createTexture(height, GL_INTENSITY32F_ARB, size);
    createTexture(color, GL_RGB, size);
}
QuadNodeVideoData::
~QuadNodeVideoData()
{
    glDeleteTextures(1, &geometry);
    glDeleteTextures(1, &height);
    glDeleteTextures(1, &color);
}

void QuadNodeVideoData::
createTexture(GLuint& texture, GLint internalFormat, uint size)
{
    glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size, size, 0,
                 GL_RGB, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    CHECK_GLA
}


QuadNodeGpuLineData::
QuadNodeGpuLineData(uint) :
    age(0)
{
    glGenTextures(1, &data);
    glBindTexture(GL_TEXTURE_1D, data);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F_ARB, Crusta::lineDataTexSize, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    CHECK_GLA

    glGenTextures(1, &coverage);
    glBindTexture(GL_TEXTURE_2D, coverage);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                 Crusta::lineCoverageTexSize, Crusta::lineCoverageTexSize, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    CHECK_GLA
}

QuadNodeGpuLineData::
~QuadNodeGpuLineData()
{
    glDeleteTextures(1, &data);
    glDeleteTextures(1, &coverage);
}


std::ostream& operator<<(std::ostream& os,
                         const QuadNodeMainData::ShapeCoverage& cov)
{
    for (QuadNodeMainData::ShapeCoverage::const_iterator lit=cov.begin();
         lit!=cov.end(); ++lit)
    {
        os << "-line " << lit->first->getId() << " XX ";
        const QuadNodeMainData::AgeStampedControlPointHandleList& handles =
            lit->second;
        for (QuadNodeMainData::AgeStampedControlPointHandleList::const_iterator
             hit=handles.begin(); hit!=handles.end(); ++hit)
        {
            if (hit==handles.begin())
                os << hit->age << "." << hit->handle;
            else
                os << " | " << hit->age << "." << hit->handle;
        }
        os << "\n";
    }

    return os;
}

END_CRUSTA
