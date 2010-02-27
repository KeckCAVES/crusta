#include <crusta/SurfaceTool.h>

#include <Comm/MulticastPipe.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <crusta/Crusta.h>

BEGIN_CRUSTA


SurfaceTool::Factory* SurfaceTool::factory = NULL;

SurfaceTool::
SurfaceTool(const Vrui::ToolFactory* iFactory,
            const Vrui::ToolInputAssignment& inputAssignment) :
    Vrui::TransformTool(iFactory, inputAssignment), CrustaComponent(NULL)
{
}

SurfaceTool::
~SurfaceTool()
{
}


Vrui::ToolFactory* SurfaceTool::
init()
{
	Vrui::TransformToolFactory* transformToolFactory =
        dynamic_cast<Vrui::TransformToolFactory*>(
            Vrui::getToolManager()->loadClass("TransformTool"));

    Factory* surfaceFactory = new Factory("SurfaceTool", "Crusta Surface",
        transformToolFactory, *Vrui::getToolManager());

	surfaceFactory->setNumDevices(1);
	surfaceFactory->setNumButtons(0, transformToolFactory->getNumButtons());
	surfaceFactory->setNumValuators(0, transformToolFactory->getNumValuators());

    Vrui::getToolManager()->addClass(surfaceFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    SurfaceTool::factory = surfaceFactory;

    return SurfaceTool::factory;
}


void SurfaceTool::
initialize()
{
    //initialize the base tool
    TransformTool::initialize();
    //disable the transformed device's glyph
    Vrui::InputGraphManager* igMan = Vrui::getInputGraphManager();
    igMan->getInputDeviceGlyph(transformedDevice).disable();
}

const Vrui::ToolFactory* SurfaceTool::
getFactory() const
{
    return factory;
}

void SurfaceTool::
frame()
{
    Vrui::InputDevice* dev = input.getDevice(0);

    if (transformEnabled)
    {
        //transform the physical frame to navigation space
        Vrui::NavTransform physicalFrame = dev->getTransformation();
        Vrui::NavTransform modelFrame    =
            Vrui::getInverseNavigationTransformation() * physicalFrame;

        //align the model frame to the surface
        Point3 surfacePoint;
        if (Vrui::isMaster())
        {
            surfacePoint = modelFrame.getOrigin();
            //snapping is done radially, no need to map to the unscaled globe
            surfacePoint = crusta->snapToSurface(surfacePoint);
            //the returned point is relative to the unscaled globe
            surfacePoint = crusta->mapToScaledGlobe(surfacePoint);

			if (Vrui::getMainPipe() != NULL)
                Vrui::getMainPipe()->write<Point3>(surfacePoint);
        }
        else
            Vrui::getMainPipe()->read<Point3>(surfacePoint);

        modelFrame = Vrui::NavTransform(Vector3(surfacePoint),
            modelFrame.getRotation(), modelFrame.getScaling());

        //transform the aligned frame back to physical space
        physicalFrame = Vrui::getNavigationTransformation() * modelFrame;
        transformedDevice->setTransformation(Vrui::TrackerState(
            physicalFrame.getTranslation(), physicalFrame.getRotation()));
    }
    else
    {
        transformedDevice->setTransformation(dev->getTransformation());
        transformedDevice->setDeviceRayDirection(dev->getDeviceRayDirection());
    }
}

void SurfaceTool::
display(GLContextData& contextData) const
{
    Vrui::NavTransform original    = input.getDevice(0)->getTransformation();
    Vrui::NavTransform transformed = transformedDevice->getTransformation();

    Point3 oPos = original.getOrigin();
    Point3 tPos = transformed.getOrigin();

    //make sure that the line is always drawn
    GLdouble depthRange[2];
    glGetDoublev(GL_DEPTH_RANGE, depthRange);
    glDepthRange(0.0, 0.0);

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
    glDepthRange(depthRange[0], depthRange[1]);
}


END_CRUSTA
