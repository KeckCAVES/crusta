#include <SpheroidGrid.h>

#include <GL/GLContextData.h>

#include <QuadTree.h>
#include <FrustumVisibility.h>
#include <ViewLod.h>

BEGIN_CRUSTA

template <class PointParam>
inline
PointParam
toSphere(
         const PointParam& p)
{
    typedef typename PointParam::Scalar Scalar;

    double len=Geometry::mag(p);
    return PointParam(Scalar(p[0]/len),Scalar(p[1]/len),Scalar(p[2]/len));
}

template <class PointParam>
inline
PointParam
centroid(
         const PointParam& p0,
         const PointParam& p1,
         const PointParam& p2)
{
    typedef typename PointParam::Scalar Scalar;

    return PointParam((p0[0]+p1[0]+p2[0])/Scalar(3),(p0[1]+p1[1]+p2[1])/Scalar(3),(p0[2]+p1[2]+p2[2])/Scalar(3));
}

SpheroidGrid::
SpheroidGrid() :
    newDataSlotId(0), dataSlotsToAdd(0)
{
}

SpheroidGrid::
~SpheroidGrid()
{
}

SpheroidGrid::ViewData::
ViewData()
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

    basePatches.resize(30);
    for (uint i=0, j=0; i<30; ++i, j+=4)
    {
        basePatches[i] = new QuadTree(Scope(
            toSphere(baseVertices[baseIndices[j]]),
            toSphere(baseVertices[baseIndices[j+1]]),
            toSphere(baseVertices[baseIndices[j+2]]),
            toSphere(baseVertices[baseIndices[j+3]])
        ));
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
	basePatches[ 0] = new QuadTree(Scope(t0,e01,f2,e03));
#else
    basePatches.resize(12);
	basePatches[ 0] = new QuadTree(Scope(t0,e01,f2,e03));
	basePatches[ 1] = new QuadTree(Scope(t0,e03,f1,e02));
	basePatches[ 2] = new QuadTree(Scope(t0,e02,f3,e01));

	basePatches[ 3] = new QuadTree(Scope(t1,e01,f3,e12));
	basePatches[ 4] = new QuadTree(Scope(t1,e12,f0,e13));
	basePatches[ 5] = new QuadTree(Scope(t1,e13,f2,e01));

	basePatches[ 6] = new QuadTree(Scope(t2,e02,f1,e23));
	basePatches[ 7] = new QuadTree(Scope(t2,e23,f0,e12));
	basePatches[ 8] = new QuadTree(Scope(t2,e12,f3,e02));

	basePatches[ 9] = new QuadTree(Scope(t3,e03,f2,e13));
	basePatches[10] = new QuadTree(Scope(t3,e13,f0,e23));
	basePatches[11] = new QuadTree(Scope(t3,e23,f1,e03));
#endif
#endif

    lod = new ViewLod;
//    lod->scale = 0.25f;
    lod->bias = -3.0f;
    visibility = new FrustumVisibility;
}

SpheroidGrid::ViewData::
~ViewData()
{
    uint numPatches = static_cast<uint>(basePatches.size());
    for (uint i=0; i<numPatches; ++i)
        delete basePatches[i];
    delete visibility;
    delete lod;
}

void SpheroidGrid::
frame()
{
}

void SpheroidGrid::
display(GLContextData& contextData) const
{
    ViewData* viewData = contextData.retrieveDataItem<ViewData>(this);

    uint numPatches = static_cast<uint>(viewData->basePatches.size());
    if (dataSlotsToAdd != 0)
    {
        for (uint i=0; i<numPatches; ++i)
            viewData->basePatches[i]->addDataSlots(dataSlotsToAdd);
        dataSlotsToAdd = 0;
    }

    viewData->visibility->frustum.setFromGL();
    viewData->lod->frustum = viewData->visibility->frustum;

    for (uint i=0; i<numPatches; ++i)
    {
        viewData->basePatches[i]->refine(*(viewData->visibility),
                                         *(viewData->lod));
    }

    for (uint i=0; i<numPatches; ++i)
        viewData->basePatches[i]->traverseLeaves(updateCallbacks, contextData);

    for (uint i=0; i<numPatches; ++i)
        viewData->basePatches[i]->traverseLeaves(displayCallbacks, contextData);
}

gridProcessing::Id SpheroidGrid::
registerDataSlot()
{
    gridProcessing::Id newId = newDataSlotId;
    ++newDataSlotId;
    ++dataSlotsToAdd;
    return newId;
}


void SpheroidGrid::
initContext(GLContextData& contextData) const
{
    ViewData* data = new ViewData;
    contextData.addDataItem(this, data);
}

END_CRUSTA
