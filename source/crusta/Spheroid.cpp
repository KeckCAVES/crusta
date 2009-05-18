#include <crusta/Spheroid.h>

#include <crusta/Triacontahedron.h>

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
    Triacontahedron triacontahedron(SPHEROID_RADIUS);

    uint numPatches = triacontahedron.getNumPatches();
    basePatches.resize(numPatches);
    for (uint i=0; i<numPatches; ++i)
        basePatches[i] = new QuadTerrain(i, triacontahedron.getScope(i));
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
