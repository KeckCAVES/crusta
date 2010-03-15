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

    GLuint getLineDataTexture(GLContextData& contextData);
    void display(GLContextData& contextData) const;

    Ptrs* lines;

    static const int lineTexSize;


//protected:
    struct GlData : public GLObject::DataItem
    {
        GlData();
        ~GlData();

        GLuint lineDataTex;
        GLuint symbolTex;

        GLShader shader;
    };

    void prepareLineData(GlData* glData) const;

//- inherited from GLObject
public:
    virtual void initContext(GLContextData& contextData) const;
};

END_CRUSTA


#endif //_PolylineRenderer_H_
