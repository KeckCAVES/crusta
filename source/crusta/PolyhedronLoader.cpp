#include <crusta/PolyhedronLoader.h>

#include <crusta/Triacontahedron.h>


BEGIN_CRUSTA


Polyhedron* PolyhedronLoader::
load(const std::string polyhedronDescriptor, double radius)
{
    Polyhedron* ret = NULL;
    if (polyhedronDescriptor == Triacontahedron(1.0).getType())
        ret = new Triacontahedron(radius);
    return ret;
}


END_CRUSTA
