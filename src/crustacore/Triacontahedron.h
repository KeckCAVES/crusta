#ifndef _Triacontahedron_H_
#define _Triacontahedron_H_


#include <crustacore/Polyhedron.h>


BEGIN_CRUSTA


/**
    Provides the basic data to build a rhombic Triacontahedron
*/
class Triacontahedron : public Polyhedron
{
//- inherited from Polyhedron
public:
    Triacontahedron(Scope::Scalar sphereRadius);

    virtual std::string getType() const;
    virtual uint        getNumPatches() const;
    virtual Scope       getScope(uint patchId) const;
    virtual void        getConnectivity(uint patchId,
                                        Connectivity connectivity[4]) const;

protected:
    Scope::Scalar radius;
};


END_CRUSTA


#endif //_Triacontahedron_H_
