#ifndef _Scope_H_
#define _Scope_H_

#include <basics.h>

BEGIN_CRUSTA

/**
    A scope is used to define an area on the surface of the earth at a 
    particular resolution.
 
    The global grid in combination with the refinement produce a tiling of the
    surface of the glob by use of scopes. Associated with each scope may be a
    set of clients that process them or provide visual representations within
    them. The area defined by the scopes can be used by these clients to query
    data from preprocessed sources (e.g. elevation or texture hierarchies).
*/
class Scope
{    
};

END_CRUSTA

#endif //_Scope_H_
