#ifndef _Triacontahedron_H_
#define _Triacontahedron_H_


#include <crustacore/Polyhedron.h>


namespace crusta {


/**
    Provides the basic data to build a rhombic Triacontahedron
*/
class Triacontahedron : public Polyhedron
{
//- inherited from Polyhedron
public:
    Triacontahedron(Scope::Scalar sphereRadius);

    virtual std::string getType() const;
    virtual size_t        getNumPatches() const;
    virtual Scope       getScope(size_t patchId) const;
    virtual void        getConnectivity(size_t patchId,
                                        Connectivity connectivity[4]) const;

protected:
    Scope::Scalar radius;
};


} //namespace crusta


#endif //_Triacontahedron_H_
