#include <crusta/SurfaceTool.h>


#include <crusta/Crusta.h>

#include <crusta/vrui.h>


namespace crusta {


SurfaceTool::Factory* SurfaceTool::factory = NULL;

SurfaceTool::
SurfaceTool(const Vrui::ToolFactory* iFactory,
            const Vrui::ToolInputAssignment& inputAssignment) :
    Vrui::TransformTool(iFactory, inputAssignment)
{
	/* Set the transformation source device: */
	if(input.getNumButtonSlots()>0)
		sourceDevice=getButtonDevice(0);
	else
		sourceDevice=getValuatorDevice(0);
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

    surfaceFactory->setNumButtons(0, true);
    surfaceFactory->setNumValuators(0, true);

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
    Vrui::InputDevice* dev = getButtonDevice(0);

    SurfacePoint p = project(dev);
    if (!projectionFailed)
    {
        Geometry::Vector<double,3> translation(p.position[0], p.position[1], p.position[2]);
        transformedDevice->setTransformation(Vrui::TrackerState(
            translation, dev->getTransformation().getRotation()));
        transformedDevice->setDeviceRay(
            dev->getDeviceRayDirection(),
            dev->getDeviceRayStart());
    }
    else
    {
        transformedDevice->setTransformation(dev->getTransformation());
        transformedDevice->setDeviceRay(dev->getDeviceRayDirection(),dev->getDeviceRayStart());
    }
}

void SurfaceTool::
display(GLContextData& contextData) const
{
    SurfaceProjector::display(contextData,
                              transformedDevice->getTransformation(),
                              getButtonDevice(0)->getTransformation());

}


void SurfaceTool::
buttonCallback(int buttonSlotIndex,
               Vrui::InputDevice::ButtonCallbackData* cbData)
{
    // disable any button callback if the projection has failed.
    if (!projectionFailed)
        TransformTool::buttonCallback(buttonSlotIndex, cbData);
}


} //namespace crusta
