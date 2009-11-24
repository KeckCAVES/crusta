#include <map/Polyline.h>

#include <GL/GLContextData.h>

BEGIN_CRUSTA


void Polyline::Renderer::
draw(GLContextData& contextData) const
{
    glBegin(GL_LINE_STRIP);
        for (Ptrs::const_iterator lIt=lines->begin(); lIt!=lines->end(); ++lIt)
        {
            Point3s& cps = lIt->getControlPoints();
            for (Point3s::const_iterator it=cps.begin(); it!=cps.end(); ++it)
                glVertex3fv(it->getComponents());
        }
    glEnd();
}
