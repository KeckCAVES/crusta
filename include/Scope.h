#ifndef _Scope_H_
#define _Scope_H_

#include <basics.h>

BEGIN_CRUSTA

/**
    A scope is used to define an area on the surface of the earth at a 
    particular resolution.
*/
class Scope
{
public:
    enum {
        LOWER_LEFT  = 0,
        LOWER_RIGHT,
        UPPER_LEFT,
        UPPER_RIGHT
    };

    Scope() {
        corners[0] = corners[1] = corners[2] = corners[3] = Point(0);
    }
    Scope(const Point& p1, const Point& p2, const Point& p3, const Point& p4) {
        corners[0] = p1; corners[1] = p2; corners[2] = p3; corners[3] = p4;
    }

    /** corner points of the scope in cartesian space in order lower-left,
        lower-right, upper-left, upper-right*/
    Point corners[4];
};

END_CRUSTA

#endif //_Scope_H_
