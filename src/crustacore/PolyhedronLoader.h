#ifndef _PolyhedronLoader_H_
#define _PolyhedronLoader_H_


#include <string>

#include <crustacore/Polyhedron.h>


BEGIN_CRUSTA


struct PolyhedronLoader
{
    static Polyhedron* load(const std::string polyhedronDescriptor,
                            double radius);
};


END_CRUSTA


#endif //_PolyhedronLoader_H_
