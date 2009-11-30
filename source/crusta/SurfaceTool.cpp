#include <crusta/SurfaceTool.h>


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
            toolManager.loadClass("TransformTool"));

    Factory* surfaceFactory = new Factory("SurfaceTool", "Crusta Surface",
        transformToolFactory, *Vrui::getToolManager());

///\todo fix the number of buttons and valuators, for now hack in 2
	surfaceFactory->setNumDevices(1);
	surfaceFactory->setNumButtons(0, 2);

    Vrui::getToolManager()->addClass(surfaceFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    return surfaceFactory;
}


const Vrui::ToolFacotry* SurfaceTool::
getFactory() const
{
    return factory;
}

void SurfaceTool::
frame()
{
    Vrui::InputDevice* device = input.getDevice(0);

    //transform the physical frame to navigation space
    Vrui::NavTransform physicalFrame = device->getTransformation();
    Vrui::NavTransform modelFrame    =
        Vrui::getInverseNavigationTransformation() * physicalFrame;

    //align the model frame to the surface
    Point3 surfacePoint;
    if (Vrui::isMaster())
        surfacePoint = Crusta::snapToSurface(modelFrame.getTranslation());
    else
        Vrui::getMainPipe()->read<Point3>(surfacePoint);

    modelFrame = Vrui::NavTransform(surfacePoint, modelFrame.getRotation(),
                                    modelFrame.getScaling());

    //transform the aligned frame back to physical space
    physicalFrame = Vrui::getNavigationTransformation() * modelFrame;
    transformedDevice->setTransformation(physicalFrame);
}

END_CRUSTA
