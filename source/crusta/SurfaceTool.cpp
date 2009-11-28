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
init(Vrui::ToolFactory* parent)
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
getFactory() const;
void SurfaceTool::
frame();

END_CRUSTA
