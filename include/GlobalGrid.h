#ifndef _GlobalGrid_H_
#define _GlobalGrid_H_

#include <basics.h>

BEGIN_CRUSTA

/**
    Base class for virtual globe manager. The task of GlobalGrid objects is to
    define the base domain, be it a flat patch, a curved one or a whole globe.
    They should also export mappings from the grid to this domain.

    The GlobalGrid is the upper most component of the Level-of-Detail (LOD)
    management scheme. It provides a set of BaseScope objects that make up the
    coarsest detail level possible. These scopes define areas of the globe and
    their neighborhood (adjoining scopes).

    \todo is associated with Refinement

    \see Refinement
    \see Scope
*/
class GlobalGrid 
{
public:
    GlobalGrid();
    
protected:
    /** refinement manager to be applied to base scopes */
    Refinement* refinement;
};

END_CRUSTA

#endif //_GlobalGrid_H_
