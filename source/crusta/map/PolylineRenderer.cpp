#include <crusta/map/PolylineRenderer.h>

#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>
#include <crusta/map/Polyline.h>
#include <crusta/QuadNodeData.h>


BEGIN_CRUSTA


PolylineRenderer::
PolylineRenderer(Crusta* iCrusta) :
    CrustaComponent(iCrusta)
{
}


void PolylineRenderer::
display(std::vector<QuadNodeMainData*>& nodes, GLContextData& contextData) const
{
    typedef std::vector<QuadNodeMainData*>                     Nodes;
    typedef QuadNodeMainData::ShapeCoverage                    Coverage;
    typedef QuadNodeMainData::AgeStampedControlPointHandleList HandleList;
    typedef Shape::ControlPointHandle                          Handle;

    CHECK_GLA

    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glPolygonOffset(1.0f, 50.0f);

    //draw the fragments of each node
    for (Nodes::iterator nit=nodes.begin(); nit!=nodes.end(); ++nit)
    {
        QuadNodeMainData*                node     = *nit;
        QuadNodeMainData::ShapeCoverage& coverage = node->lineCoverage;
        const Point3&                    centroid = node->centroid;

        //setup the transformation for the given node
        glPushMatrix();
        Vrui::Vector centroidTranslation(centroid[0], centroid[1], centroid[2]);
        Vrui::NavTransform nav =
            Vrui::getDisplayState(contextData).modelviewNavigational;
        nav *= Vrui::NavTransform::translate(centroidTranslation);
        glLoadMatrix(nav);


        //iterate through all the lines for the given node
        for (Coverage::iterator lit=coverage.begin(); lit!=coverage.end();
             ++lit)
        {
            const Shape* const shape = lit->first;
            HandleList& handles      = lit->second;
            assert(handles.size() > 0);
            assert(dynamic_cast<const Polyline*>(shape) != NULL);

            Color symbolColor    = shape->getSymbol().color;
            Color symbolColorDim = symbolColor;
            symbolColorDim[3]   *= 0.33f;

            //iterate over all the fragments of a line
            for (HandleList::iterator hit=handles.begin(); hit!=handles.end();
                 ++hit)
            {
                Handle cur  = hit->handle;
                Handle next = cur; ++cur;

                //generate proper coordinates for the fragment
                const Point3 curP  = crusta->mapToScaledGlobe(cur->pos);
                const Point3 nextP = crusta->mapToScaledGlobe(next->pos);
                Point3f curPf(curP[0]-centroid[0], curP[1]-centroid[1],
                              curP[2]-centroid[2]);
                Point3f nextPf(nextP[0]-centroid[0], nextP[1]-centroid[1],
                               nextP[2]-centroid[2]);

                //draw visible fragment
                glDepthFunc(GL_LEQUAL);
                glLineWidth(2.0);
                glColor(symbolColor);
                glBegin(GL_LINES);
                    glVertex(curPf);
                    glVertex(nextPf);
                glEnd();

                //display hidden fragment
                glDepthFunc(GL_GREATER);
                glLineWidth(1.0);
                glColor(symbolColorDim);
                glBegin(GL_LINES);
                    glVertex(curPf);
                    glVertex(nextPf);
                glEnd();

                CHECK_GLA
            }
        }

        //restore the transformation
        glPopMatrix();
    }

    glPopAttrib();
    CHECK_GLA
}


END_CRUSTA
