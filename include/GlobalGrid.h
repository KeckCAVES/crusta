#ifndef _GlobalGrid_H_
#define _GlobalGrid_H_

#include <basics.h>
#include <gridProcessing.h>

class GLContextData;

BEGIN_CRUSTA

/**
    Base class for virtual globe manager. The task of GlobalGrid objects is to
    define the base domain, be it a flat patch, a curved one or a whole globe.

    The GlobalGrid is the upper most component of the Level-of-Detail (LOD)
    management scheme. It provides a set of Refinement objects that make up the
    coarsest detail level possible. These scopes define areas of the globe and
    their neighborhood (adjoining scopes).

    \see Refinement
    \see Scope
*/
class GlobalGrid
{
public:
    virtual ~GlobalGrid() {}

    virtual void frame() = 0;
    virtual void display(GLContextData& contextData) const = 0;

    virtual gridProcessing::Id registerDataSlot() = 0;
    void registerScopeCallback(gridProcessing::Phase phase,
        const gridProcessing::ScopeCallback callback);

protected:
///\todo because of const in display method, need to make mutable. Fix it!!
    mutable gridProcessing::ScopeCallbacks updateCallbacks;
    mutable gridProcessing::ScopeCallbacks displayCallbacks;
};

END_CRUSTA

#endif //_GlobalGrid_H_
