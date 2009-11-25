#ifndef _PolylineRenderer_H_
#define _PolylineRenderer_H_

#include <crusta/basics.h>


class GLContextData;


BEGIN_CRUSTA


class Polyline;


class PolylineRenderer
{
public:
    typedef std::vector<Polyline*> Ptrs;

    void draw(GLContextData& contextData) const;
    
    Ptrs* lines;
};

END_CRUSTA


#endif //_PolylineRenderer_H_
