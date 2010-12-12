#include <crusta/SurfaceTool.h>


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
    Vrui::TransformTool(iFactory, inputAssignment)
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

    Factory* surfaceFactory = new Factory("CrustaSurfaceTool", "Crusta Surface",
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
        SurfacePoint p = project(dev);
        if (!projectionFailed)
        {
            Vector3 translation(p.position[0], p.position[1], p.position[2]);
            transformedDevice->setTransformation(Vrui::TrackerState(
                translation, dev->getTransformation().getRotation()));
            transformedDevice->setDeviceRayDirection(
                dev->getDeviceRayDirection());
        }
    }

    if (!transformEnabled || projectionFailed)
    {
        transformedDevice->setTransformation(dev->getTransformation());
        transformedDevice->setDeviceRayDirection(dev->getDeviceRayDirection());
    }
}

void SurfaceTool::
display(GLContextData& contextData) const
{
    SurfaceProjector::display(contextData,
                              transformedDevice->getTransformation(),
                              input.getDevice(0)->getTransformation());

}


void SurfaceTool::
buttonCallback(int deviceIndex, int deviceButtonIndex,
               Vrui::InputDevice::ButtonCallbackData* cbData)
{
    // disable any button callback if the projection has failed.
    if (!projectionFailed)
        TransformTool::buttonCallback(deviceIndex, deviceButtonIndex, cbData);
}


END_CRUSTA
