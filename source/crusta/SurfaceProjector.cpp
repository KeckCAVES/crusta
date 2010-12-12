#include <crusta/SurfaceProjector.h>


#include <Comm/MulticastPipe.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Vrui.h>

#include <crusta/Crusta.h>
#include <crusta/CrustaSettings.h>


BEGIN_CRUSTA


SurfaceProjector::
SurfaceProjector() :
    CrustaComponent(NULL), projectionFailed(false)
{
}

SurfacePoint SurfaceProjector::
project(Vrui::InputDevice* device)
{
    //transform the physical frame to navigation space
    Vrui::NavTransform physicalFrame = device->getTransformation();
    Vrui::NavTransform modelFrame    =
        Vrui::getInverseNavigationTransformation() * physicalFrame;

    //align the model frame to the surface
    SurfacePoint surfacePoint;
    if (Vrui::isMaster())
    {
        if (SETTINGS->surfaceProjectorRayIntersect)
        {
            Vrui::Vector rayDir = device->getRayDirection();
            rayDir = Vrui::getInverseNavigationTransformation().transform(
                rayDir);

            Ray ray(modelFrame.getOrigin(), rayDir);
            surfacePoint = crusta->intersect(ray);
        }
        else
        {
            //snapping is done radially, no need to map to the unscaled globe
            surfacePoint = crusta->snapToSurface(modelFrame.getOrigin());
            //the returned point is relative to the unscaled globe
            surfacePoint.position =
                crusta->mapToScaledGlobe(surfacePoint.position);
        }

        if (Vrui::getMainPipe() != NULL)
            Vrui::getMainPipe()->write<SurfacePoint>(surfacePoint);
    }
    else
        Vrui::getMainPipe()->read<SurfacePoint>(surfacePoint);

    if (!surfacePoint.isValid())
    {
        projectionFailed = true;
PROJECTION_FAILED = true;
    }
    else
    {
        //transform the position back to physical space
        surfacePoint.position = Vrui::getNavigationTransformation().transform(
                                surfacePoint.position);
        projectionFailed = false;
PROJECTION_FAILED = false;
    }

    return surfacePoint;
}


void SurfaceProjector::
display(GLContextData&,
        const Vrui::NavTransform& original,
        const Vrui::NavTransform& transformed) const
{
    if (projectionFailed)
    {
///\todo look at ScreenLocatorTool to see how to better display this
        glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);

        glLineWidth(3.0);
        glColor3f(0.5f, 0.2f, 0.1f);

        Point3 o = transformed.getOrigin();
        //slightly offset the origin (mouse on near plane)
        o += 0.001 * transformed.getDirection(1);

        Vector3 x = 0.005*transformed.getDirection(0);
        Vector3 z = 0.005*transformed.getDirection(2);

        glBegin(GL_LINES);
            glVertex3dv((o-x+z).getComponents());
            glVertex3dv((o+x-z).getComponents());
            glVertex3dv((o+x+z).getComponents());
            glVertex3dv((o-x-z).getComponents());
        glEnd();

        glPopAttrib();
    }
    else
    {
        Point3 oPos = original.getOrigin();
        Point3 tPos = transformed.getOrigin();

        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);

        if (Geometry::dist(oPos, tPos) < 1.0)
            glColor3f(0.3f, 0.6f, 0.1f);
        else
            glColor3f(0.3f, 0.0f, 0.0f);

        glBegin(GL_LINES);
            glVertex3dv(oPos.getComponents());
            glVertex3dv(tPos.getComponents());
        glEnd();

        glPopAttrib();
    }
}


END_CRUSTA
