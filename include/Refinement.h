#ifndef _Refinement_H_
#define _Refinement_H_

#include <basics.h>

BEGIN_CRUSTA

/**
    Base class for refinement algorithms on base patches.
 
    \todo maintains scope hierarchy
    \todo exports changes to active scopes
    \todo queryable active scopes
    \todo queryable scope neighborhood
*/
class Refinement
{
public:
    Refinement();
    
protected:
    /** update the refinement based on the given guide */
};

END_CRUSTA

#endif //_Refinement_H_
