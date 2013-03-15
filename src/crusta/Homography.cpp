#include <crusta/Homography.h>

#define HOMOGRAPHY_VERBOSE 0

#if HOMOGRAPHY_VERBOSE
#include <iostream>
#include <iomanip>
#endif //HOMOGRAPHY_VERBOSE

#include <Misc/Utility.h>
#include <Math/Math.h>
#include <Math/Random.h>
#define NONSTANDARD_TEMPLATES
#include <Geometry/ComponentArray.h>
#include <Geometry/Point.h>
#include <Geometry/HVector.h>
#include <Geometry/Matrix.h>


namespace crusta {


template <class ScalarParam,int dimensionParam>
inline
Geometry::ComponentArray<ScalarParam,dimensionParam>
calcNullSpace(
    const Geometry::Matrix<ScalarParam,dimensionParam,dimensionParam>& m)
{
    /* Copy the matrix into a temporary: */
    Geometry::Matrix<double,dimensionParam,dimensionParam> mt;
    for(int i=0;i<dimensionParam;++i)
    {
        for(int j=0;j<dimensionParam;++j)
            mt(i,j)=double(m(i,j));
    }

    /* Find the null space of the matrix: */
    int rowIndices[dimensionParam];
    for(int i=0;i<dimensionParam;++i)
        rowIndices[i]=i;
    int rank=0;
    for(int step=0;step<dimensionParam-1;++step)
    {
        /* Find the full pivot: */
        double pivot=Math::abs(mt(step,step));
        int pivotRow=step;
        int pivotCol=step;
        for(int i=step;i<dimensionParam;++i)
            for(int j=step;j<dimensionParam;++j)
            {
            double val=Math::abs(mt(i,j));
            if(pivot<val)
            {
                pivot=val;
                pivotRow=i;
                pivotCol=j;
            }
        }

        /* Bail out if the rest of the matrix is all zeros: */
        if(pivot==0.0)
            break;

        ++rank;

        /* Swap current and pivot rows if necessary: */
        if(pivotRow!=step)
        {
            /* Swap rows step and pivotRow: */
            for(int j=0;j<dimensionParam;++j)
                Misc::swap(mt(step,j),mt(pivotRow,j));
        }

        /* Swap current and pivot columns if necessary: */
        if(pivotCol!=step)
        {
            /* Swap columns step and pivotCol: */
            for(int i=0;i<dimensionParam;++i)
                Misc::swap(mt(i,step),mt(i,pivotCol));
            Misc::swap(rowIndices[step],rowIndices[pivotCol]);
        }

        /* Combine all rows with the current row: */
        for(int i=step+1;i<dimensionParam;++i)
        {
            /* Combine rows i and step: */
            double factor=-mt(i,step)/mt(step,step);
            for(int j=step+1;j<dimensionParam;++j)
                mt(i,j)+=mt(step,j)*factor;
        }
    }

    if(mt(dimensionParam-1,dimensionParam-1)!=0.0)
        ++rank;

#if HOMOGRAPHY_VERBOSE
    std::cout<<"Matrix rank: "<<rank<<std::endl;
#endif //HOMOGRAPHY_VERBOSE

    /* Calculate the swizzled result using backsubstitution: */
    double x[dimensionParam];
    x[dimensionParam-1]=1.0;
    double xmag=1.0;
    for(int i=dimensionParam-2;i>=0;--i)
    {
        x[i]=0.0;
        for(int j=i+1;j<dimensionParam;++j)
            x[i]-=mt(i,j)*x[j];
        if(mt(i,i)!=0.0)
            x[i]/=mt(i,i);
        xmag+=Math::sqr(x[i]);
    }
    xmag=Math::sqrt(xmag);

    /* Unswizzle and normalize the result: */
    Geometry::ComponentArray<ScalarParam,dimensionParam> result;
    for(int i=0;i<dimensionParam;++i)
        result[rowIndices[i]]=ScalarParam(x[i]/xmag);
    return result;
}


void Homography::
setSource(const HVector& p0, const HVector& p1, const HVector& p2,
          const HVector& p3, const HVector& p4)
{
    sources[0] = p0;
    sources[1] = p1;
    sources[2] = p2;
    sources[3] = p3;
    sources[4] = p4;
}

void Homography::
setDestination(const HVector& p0, const HVector& p1, const HVector& p2,
               const HVector& p3, const HVector& p4)
{
    destinations[0] = p0;
    destinations[1] = p1;
    destinations[2] = p2;
    destinations[3] = p3;
    destinations[4] = p4;
}


void Homography::
computeProjective()
{
    /* Create the linear equation system: */
    Geometry::Matrix<Scalar,16,16> a;
    for (int i=0; i<16; ++i)
    {
        for (int j=0; j<16; ++j)
            a(i,j) = 0.0;
    }
    for (int i=0; i<5; ++i)
    {
        for (int j=0; j<3; ++j)
        {
            double w = -destinations[i][j]/destinations[i][3];
            for (int k=0; k<4; ++k)
            {
                a(i*3+j, j*4+k) = sources[i][k];
                a(i*3+j, 3*4+k) = w*sources[i][k];
            }
        }
    }

#if HOMOGRAPHY_VERBOSE
    std::cout << "Homography: source points: " << std::endl;
    for (int i=0; i<5; ++i)
    {
        std::cout<<"("<<sources[i][0]<<","<<sources[i][1]<<","<<
                        sources[i][2]<<")\n";
    }
    std::cout << std::endl << "Homography: destinations points: " << std::endl;
    for (int i=0; i<5; ++i)
    {
        std::cout << "("<<destinations[i][0]<<","<<destinations[i][1]<<
                     ","<<destinations[i][2]<<")"<<std::endl;
    }

    std::cout <<std::endl<< "Homography: Equation matrix: " << std::endl;
    for(int i=0;i<16;++i)
    {
        for(int j=0;j<16;++j)
            std::cout<<" "<<std::setw(3)<<a(i,j);
        std::cout<<std::endl;
    }
    std::cout<<std::endl;
#endif //HOMOGRAPHY_VERBOSE

    /* Solve the linear system: */
    Geometry::ComponentArray<Scalar, 16> temp;
    temp = calcNullSpace(a);

    //copy into the projective transformation
    for(int i=0;i<4;++i)
    {
        for(int j=0;j<4;++j)
            projective.getMatrix()(i,j)=temp[i*4+j];
    }

#if HOMOGRAPHY_VERBOSE
    std::cout << "Projective matrix: " << std::endl;
    for(int i=0;i<4;++i)
    {
        for(int j=0;j<4;++j)
            std::cout << projective(i,j) << " ";
        std::cout << std::endl;
    }
#endif //HOMOGRAPHY_VERBOSE
}

const Homography::Projective& Homography::
getProjective() const
{
    return projective;
}

#if 0
int main(void)
{
    /* Create a set of eye points and a corresponding set of clip points: */
    Point eye[]=
    {
        Point(-1.0,-1.0, 1.0),Point( 1.0,-1.0, 1.0),Point(-1.0, 1.0, 1.0),Point(-10.0,-10.0, 10.0),Point( 10.0, 10.0, 10.0),
        Point( 1.0, 1.0, 1.0),Point(-10.0, 10.0, 10.0),Point( 10.0,-10.0, 10.0)
    };
    Point clip[]=
    {
        Point(-1.0,-1.0,-1.0),Point( 1.0,-1.0,-1.0),Point(-1.0, 1.0,-1.0),Point(-1.0,-1.0, 1.0),Point( 1.0, 1.0, 1.0),
        Point( 1.0, 1.0,-1.0),Point(-1.0, 1.0, 1.0),Point( 1.0,-1.0, 1.0)
    };

    /* Check the result: */
    Geometry::ComponentArray<double,16> y=a*x;
    std::cout<<"Result check (should be zero): "<<y.mag()<<std::endl;

    /* Test the projection matrix on all the eye points: */
    for(int i=0;i<8;++i)
    {
        HPoint clipp=p.transform(HPoint(eye[i]));
        std::cout<<"("<<clipp[0]<<", "<<clipp[1]<<", "<<clipp[2]<<", "<<clipp[3]<<"), ";
        Point clippp=clipp.toPoint();
        std::cout<<"("<<clippp[0]<<", "<<clippp[1]<<", "<<clippp[2]<<")"<<std::endl;
    }

    return 0;
}
#endif

} //namespace crusta
