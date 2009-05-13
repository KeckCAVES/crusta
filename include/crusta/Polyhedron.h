#ifndef _Polyhedron_H_
#define _Polyhedron_H_

#include <crusta/Scope.h>

BEGIN_CRUSTA

/**
    Interface class for specializations that provide the geometry and
    connectivity for a sphere approximation based on square patches.
*/
class Polyhedron
{
public:
    typedef uint Connectivity[2];

    virtual ~Polyhedron() {}
    
    virtual uint  getNumPatches() = 0;
    virtual Scope getScope(uint patchId) = 0;
    virtual void  getConnectivity(uint patchId, Connectivity connectivity[4])=0;
};

END_CRUSTA

#endif //_Polyhedron_H_
