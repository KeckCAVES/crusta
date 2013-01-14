#ifndef _Polyhedron_H_
#define _Polyhedron_H_


#include <string>

#include <crustacore/Scope.h>


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

    virtual std::string getType() const = 0;
    virtual uint        getNumPatches() const = 0;
    virtual Scope       getScope(uint patchId) const = 0;
    virtual void        getConnectivity(uint patchId,
                                        Connectivity connectivity[4]) const = 0;
};


END_CRUSTA


#endif //_Polyhedron_H_
