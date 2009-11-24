#ifndef _Polyline_H_
#define _Polyline_H_

#include <crusta/Shape.h>


class GLContextData;

BEGIN_CRUSTA


class Polyline : public Shape
{
public:
    typedef std::vector<Polyline*> Ptrs;

    class Renderer
    {
    public:
        void draw(GLContextData& contextData) const;

        Ptrs* lines;
    };
};

#endif //_Polyline_H_
