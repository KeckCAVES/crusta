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
public:
    enum {
        UPPER_LEFT = 0,
        LOWER_LEFT,
        LOWER_RIGHT,
        UPPER_RIGHT
    };

    Scope() {
        corners[0] = corners[1] = corners[2] = corners[3] = Point(0);
    }
    Scope(const Point& p1, const Point& p2, const Point& p3, const Point& p4) {
        corners[0] = p1; corners[1] = p2; corners[2] = p3; corners[3] = p4;
    }

    /** corner points of the scope in cartesian space in counter-clockwise
        order starting at the top-left */
    Point corners[4];
};

END_CRUSTA

#endif //_Scope_H_
