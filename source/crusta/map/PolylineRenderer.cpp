#include <crusta/map/PolylineRenderer.h>

#include <GL/gl.h>
#include <GL/GLContextData.h>

#include <crusta/map/Polyline.h>


BEGIN_CRUSTA


void PolylineRenderer::
draw(GLContextData& contextData) const
{
    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);
    glPushAttrib(GL_ENABLE_BIT);

    glDisable(GL_LIGHTING);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    for (Ptrs::const_iterator lIt=lines->begin(); lIt!=lines->end(); ++lIt)
    {
        const Point3s& cps = (*lIt)->getControlPoints();
        if (cps.size() < 2)
            continue;
        glBegin(GL_LINE_STRIP);
        for (Point3s::const_iterator it=cps.begin(); it!=cps.end(); ++it)
            glVertex3f((*it)[0], (*it)[1], (*it)[2]);
        glEnd();
    }

///\todo remove
    for (Ptrs::const_iterator lIt=lines->begin(); lIt!=lines->end(); ++lIt)
    {
        const Point3s& cps = (*lIt)->getControlPoints();
        glBegin(GL_POINTS);
        for (Point3s::const_iterator it=cps.begin(); it!=cps.end(); ++it)
            glVertex3f((*it)[0], (*it)[1], (*it)[2]);
        glEnd();
    }

    glPopAttrib();
    glActiveTexture(activeTexture);
}


END_CRUSTA
