#include <crusta/map/PolylineRenderer.h>

#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Math/Constants.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>
#include <crusta/map/Polyline.h>


BEGIN_CRUSTA


PolylineRenderer::
PolylineRenderer(Crusta* iCrusta) :
    CrustaComponent(iCrusta)
{
}

void PolylineRenderer::
display(GLContextData& contextData) const
{
    CHECK_GLA
    if (lines->size()<1)
        return;

    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);
    glActiveTexture(GL_TEXTURE0);

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_LINE_BIT | GL_POLYGON_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_POLYGON_OFFSET_LINE);

    glPolygonOffset(1.0f, 50.0f);

    //compute the vertically scaled control points and centroids for display
    int numLines = static_cast<int>(lines->size());
    Point3s centroids;
    centroids.resize(numLines, Point3(0,0,0));
    std::vector<Point3s> controlPoints;
    controlPoints.resize(numLines);

    for (int i=0; i<numLines; ++i)
    {
        const Point3s& cps = (*lines)[i]->getControlPoints();
        int numPoints      = static_cast<int>(cps.size());
        controlPoints[i].resize(numPoints);
        for (int j=0; j<numPoints; ++j)
        {
            Point3 point = crusta->mapToScaledGlobe(cps[j]);
            for (int k=0; k<3; ++k)
            {
                controlPoints[i][j][k] = point[k];
                centroids[i][k]       += point[k];
            }
        }
        double norm = 1.0 / numPoints;
        for (int k=0; k<3; ++k)
            centroids[i][k] *= norm;
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    for (int i=0; i<numLines; ++i)
    {
        const Point3s& cps = controlPoints[i];
        if (cps.size() < 1)
            continue;

        glPushMatrix();
        Vrui::Vector centroidTranslation(centroids[i][0], centroids[i][1],
                                         centroids[i][2]);
        Vrui::NavTransform nav =
            Vrui::getDisplayState(contextData).modelviewNavigational;
        nav *= Vrui::NavTransform::translate(centroidTranslation);
        glLoadMatrix(nav);

        //draw visible lines
        glDepthFunc(GL_LEQUAL);
        glLineWidth(2.0);
        Color symbolColor = (*lines)[i]->getSymbol().color;
        glColor4fv(symbolColor.getComponents());
        glBegin(GL_LINE_STRIP);
        for (Point3s::const_iterator it=cps.begin(); it!=cps.end(); ++it)
        {
            glVertex3f((*it)[0] - centroids[i][0],
                       (*it)[1] - centroids[i][1],
                       (*it)[2] - centroids[i][2]);
        }
        glEnd();

        //display hidden lines
        glDepthFunc(GL_GREATER);
        glLineWidth(1.0);
        symbolColor[3] *= 0.33f;
        glColor4fv(symbolColor.getComponents());
        glBegin(GL_LINE_STRIP);
        for (Point3s::const_iterator it=cps.begin(); it!=cps.end(); ++it)
        {
            glVertex3f((*it)[0] - centroids[i][0],
                       (*it)[1] - centroids[i][1],
                       (*it)[2] - centroids[i][2]);
        }
        glEnd();

        glPopMatrix();
        CHECK_GLA
    }

    glPopAttrib();
    glActiveTexture(activeTexture);
    CHECK_GLA
}


END_CRUSTA
