#include <crustacore/PolyhedronLoader.h>

#include <crustacore/Triacontahedron.h>


namespace crusta {


Polyhedron* PolyhedronLoader::
load(const std::string polyhedronDescriptor, double radius)
{
    Polyhedron* ret = NULL;
    if (polyhedronDescriptor == Triacontahedron(1.0).getType())
        ret = new Triacontahedron(radius);
    return ret;
}


} //namespace crusta
