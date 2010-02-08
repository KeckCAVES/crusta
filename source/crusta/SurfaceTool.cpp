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

///\todo fix the number of buttons and valuators, for now hack in 2
	surfaceFactory->setNumDevices(1);
	surfaceFactory->setNumButtons(0, 2);

    Vrui::getToolManager()->addClass(surfaceFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    return surfaceFactory;
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
            surfacePoint =
                crusta->snapToSurface(modelFrame.getOrigin());
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

    //transform the physical frames to navigation space
    original    = Vrui::getInverseNavigationTransformation() * original;
    transformed = Vrui::getInverseNavigationTransformation() * transformed;
    
    Point3 oPos = original.getOrigin();
    Point3 tPos = transformed.getOrigin();

    if (Geometry::dist(oPos, tPos) > -9000)
    {
        GLint activeTexture;
        glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);
        glActiveTexture(GL_TEXTURE0);

        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);

        glBegin(GL_LINES);
            glColor3f(1.0, 0.0, 0.0);
            glVertex3dv(oPos.getComponents());
            glVertex3dv(tPos.getComponents());
        glEnd();

        glPopAttrib();
        glActiveTexture(activeTexture);
    }
}


END_CRUSTA
