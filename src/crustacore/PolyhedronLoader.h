#ifndef _PolyhedronLoader_H_
#define _PolyhedronLoader_H_


#include <string>

#include <crustacore/Polyhedron.h>


namespace crusta {


struct PolyhedronLoader
{
    static Polyhedron* load(const std::string polyhedronDescriptor,
                            double radius);
};


} //namespace crusta


#endif //_PolyhedronLoader_H_
