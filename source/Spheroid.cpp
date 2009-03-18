#include <Spheroid.h>

BEGIN_CRUSTA

template <class PointParam>
inline
PointParam
toSphere(const PointParam& p)
{
    typedef typename PointParam::Scalar Scalar;
    
    double len=Geometry::mag(p);
    return PointParam(Scalar(p[0]/len),Scalar(p[1]/len),Scalar(p[2]/len));
}

template <class PointParam>
inline
PointParam
centroid(const PointParam& p0,
         const PointParam& p1,
         const PointParam& p2)
{
    typedef typename PointParam::Scalar Scalar;
    
    return PointParam((p0[0]+p1[0]+p2[0])/Scalar(3),
                      (p0[1]+p1[1]+p2[1])/Scalar(3),
                      (p0[2]+p1[2]+p2[2])/Scalar(3));
}

Spheroid::
Spheroid()
{
#if 1
    //Triacontahedron from Mathematica 6-7.
    static const Point baseVertices[] = {
        Point(0,0,0), //dummy because Mathematica indices start a 1 not 0
        Point(0.0, 0.0, -1.61803),
        Point(0.0, 0.0, 1.61803),
        Point(0.276393, -0.850651, 1.17082),
        Point(0.276393, 0.850651, 1.17082),
        Point(0.894427, 0.0, 1.17082),
        Point(1.17082, -0.850651, 0.723607),
        Point(1.17082, -0.850651, -0.276393),
        Point(1.17082, 0.850651, 0.723607),
        Point(1.17082, 0.850651, -0.276393),
        Point(-0.894427, 0.0, -1.17082),
        Point(-0.447214, -1.37638, 0.723607),
        Point(-0.447214, -1.37638, -0.276393),
        Point(-0.447214, 1.37638, 0.723607),
        Point(-0.447214, 1.37638, -0.276393),
        Point(0.447214, -1.37638, 0.276393),
        Point(0.447214, -1.37638, -0.723607),
        Point(0.447214, 1.37638, 0.276393),
        Point(0.447214, 1.37638, -0.723607),
        Point(-1.44721, 0.0, 0.723607),
        Point(-1.44721, 0.0, -0.276393),
        Point(-0.723607, -0.525731, 1.17082),
        Point(-0.723607, 0.525731, 1.17082),
        Point(0.723607, -0.525731, -1.17082),
        Point(0.723607, 0.525731, -1.17082),
        Point(1.44721, 0.0, 0.276393),
        Point(1.44721, 0.0, -0.723607),
        Point(-1.17082, -0.850651, 0.276393),
        Point(-1.17082, -0.850651, -0.723607),
        Point(-1.17082, 0.850651, 0.276393),
        Point(-1.17082, 0.850651, -0.723607),
        Point(-0.276393, -0.850651, -1.17082),
        Point(-0.276393, 0.850651, -1.17082)
    };
#if 1
//mirrored-Z indices
    static const uint baseIndices[] = {
        16, 15, 12, 11,         14, 13, 18, 17,
        10, 28, 30, 20,         8, 5, 25, 6,
        12, 28, 16, 31,         32, 30, 18, 14,
        6, 3, 15, 11,           8, 17, 4, 13,
        11, 21, 27, 19,         13, 29, 22, 19,
        7, 16, 26, 23,          24, 18, 26, 9,
        12, 11, 28, 27,         30, 29, 14, 13,
        7, 6, 16, 15,           18, 17, 9, 8,
        2, 22, 21, 19,          23, 1, 26, 24,
        3, 2, 11, 21,           4, 13, 2, 22,
        16, 31, 23, 1,          1, 32, 24, 18,
        31, 28, 1, 10,          10, 30, 1, 32,
        6, 5, 3, 2,             8, 4, 5, 2,
        28, 27, 20, 19,         20, 19, 30, 29,
        26, 25, 7, 6,           9, 8, 26, 25
    };
#else
//counter-clockwise indices
    static const uint baseIndices[] = {
        16, 15, 11, 12,         14, 13, 17, 18,
        10, 28, 20, 30,         8, 5, 6, 25,
        12, 28, 31, 16,         32, 30, 14, 18,
        6, 3, 11, 15,           8, 17, 13, 4,
        11, 21, 19, 27,         13, 29, 19, 22,
        7, 16, 23, 26,          24, 18, 9, 26,
        12, 11, 27, 28,         30, 29, 13, 14,
        7, 6, 15, 16,           18, 17, 8, 9,
        2, 22, 19, 21,          23, 1, 24, 26,
        3, 2, 21, 11,           4, 13, 22, 2,
        16, 31, 1, 23,          1, 32, 18, 24,
        31, 28, 10, 1,          10, 30, 32, 1,
        6, 5, 2, 3,             8, 4, 2, 5,
        28, 27, 19, 20,         20, 19, 29, 30,
        26, 25, 6, 7,           9, 8, 25, 26
    };
#endif
    
    basePatches.resize(30);
    for (uint i=0, j=0; i<30; ++i, j+=4)
    {
        basePatches[i] =
        new QuadTerrain(i, Scope(toSphere(baseVertices[baseIndices[j]]),
                                 toSphere(baseVertices[baseIndices[j+1]]),
                                 toSphere(baseVertices[baseIndices[j+2]]),
                                 toSphere(baseVertices[baseIndices[j+3]])));
    }
#else
	/****************************************************************
     Create a rhombic dodecahedron by subdividing a tetrahedron with a
     single Catmull-Clark step:
     ****************************************************************/
    
	/* Create a tetrahedron: */
	Point t0=toSphere(Point(-1,-1,-1));
	Point t1=toSphere(Point( 1, 1,-1));
	Point t2=toSphere(Point(-1, 1, 1));
	Point t3=toSphere(Point( 1,-1, 1));
    
	/* Calculate the six edge points: */
	Point e01=toSphere(Geometry::mid(t0,t1));
	Point e02=toSphere(Geometry::mid(t0,t2));
	Point e03=toSphere(Geometry::mid(t0,t3));
	Point e12=toSphere(Geometry::mid(t1,t2));
	Point e13=toSphere(Geometry::mid(t1,t3));
	Point e23=toSphere(Geometry::mid(t2,t3));
    
	/* Calculate the four face points: */
	Point f0=toSphere(centroid(t1,t2,t3));
	Point f1=toSphere(centroid(t0,t2,t3));
	Point f2=toSphere(centroid(t0,t1,t3));
	Point f3=toSphere(centroid(t0,t1,t2));
    
	/********************************************************************
     Create the twelve base quads, as four fans of three quads each around
     each tetrahedron vertex:
     ********************************************************************/
    
#if 1
    basePatches.resize(1);
	basePatches[ 0] = new QuadTerrain(0, Scope(t0,e01,e03,f2));
///\todo debug generate a flat surface to debug light direction
#if 0
    basePatches[ 0] = new QuadTerrain(0, Scope(toSphere(Point(-1,-1,-1)),
                                               toSphere(Point( 1,-1,-1)),
                                               toSphere(Point(-1,-1, 1)),
                                               toSphere(Point( 1,-1, 1))));
#endif
#else
    basePatches.resize(12);
	basePatches[ 0] = new QuadTerrain( 0, Scope(t0,e01,f2,e03));
	basePatches[ 1] = new QuadTerrain( 1, Scope(t0,e03,f1,e02));
	basePatches[ 2] = new QuadTerrain( 2, Scope(t0,e02,f3,e01));
    
	basePatches[ 3] = new QuadTerrain( 3, Scope(t1,e01,f3,e12));
	basePatches[ 4] = new QuadTerrain( 4, Scope(t1,e12,f0,e13));
	basePatches[ 5] = new QuadTerrain( 5, Scope(t1,e13,f2,e01));
    
	basePatches[ 6] = new QuadTerrain( 6, Scope(t2,e02,f1,e23));
	basePatches[ 7] = new QuadTerrain( 7, Scope(t2,e23,f0,e12));
	basePatches[ 8] = new QuadTerrain( 8, Scope(t2,e12,f3,e02));
    
	basePatches[ 9] = new QuadTerrain( 9, Scope(t3,e03,f2,e13));
	basePatches[10] = new QuadTerrain(10, Scope(t3,e13,f0,e23));
	basePatches[11] = new QuadTerrain(11, Scope(t3,e23,f1,e03));
#endif
#endif
}

Spheroid::
~Spheroid()
{
    for (TerrainPtrs::iterator it=basePatches.begin(); it!=basePatches.end();
         ++it)
    {
        delete *it;
    }
}


void Spheroid::
display(GLContextData& contextData) const
{
    for (TerrainPtrs::const_iterator it=basePatches.begin(); it!=basePatches.end();
         ++it)
    {
        (*it)->display(contextData);
    }
}


END_CRUSTA
