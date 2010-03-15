#ifndef _PolylineTraversalFunctor_H_
#define _PolylineTraversalFunctor_H_


#include <crusta/basics.h>


BEGIN_CRUSTA


class QuadNodeMainData;

class PolylineTraversalFunctor
{
public:
    virtual void operator()(const Point3s::const_iterator& cp,
                            QuadNodeMainData* node) = 0;
};


END_CRUSTA


#endif //_PolylineTraversalFunctor_H_
