#ifndef _PolylineRenderer_H_
#define _PolylineRenderer_H_


#include <GL/GLObject.h>
#include <GL/GLShader.h>

#include <crusta/basics.h>
#include <crusta/CrustaComponent.h>


class GLContextData;


BEGIN_CRUSTA


class Polyline;


class PolylineRenderer : public CrustaComponent, public GLObject
{
public:
    typedef std::vector<Polyline*> Ptrs;

    PolylineRenderer(Crusta* iCrusta);

    void display(GLContextData& contextData) const;

    Ptrs* lines;

protected:
    typedef Geometry::Vector<float, 3> Tangent;
    typedef std::vector<Tangent> Tangents;

    struct GlData : public GLObject::DataItem
    {
        GlData();
        ~GlData();

        GLuint controlPointTex;
        GLuint tangentTex;
        GLuint symbolTex;

        GLShader shader;

        GLint windowPosUniform;
        GLint rcpWindowSizeUniform;

        GLint numSegmentsUniform;
        GLint inverseMVPUniform;
    };

    int  prepareLineData(GlData* glData) const;

    mutable Tangents tangents;

//- inherited from GLObject
public:
   	virtual void initContext(GLContextData& contextData) const;
};

END_CRUSTA


#endif //_PolylineRenderer_H_
