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
    typedef size_t Connectivity[2];

    virtual ~Polyhedron() {}

    virtual std::string getType() const = 0;
    virtual size_t        getNumPatches() const = 0;
    virtual Scope       getScope(size_t patchId) const = 0;
    virtual void        getConnectivity(size_t patchId,
                                        Connectivity connectivity[4]) const = 0;
};


END_CRUSTA


#endif //_Polyhedron_H_
