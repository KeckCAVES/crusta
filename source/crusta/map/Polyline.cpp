#include <crusta/map/Polyline.h>


BEGIN_CRUSTA


void PolylineTool::
init()
{
    Factory* polylineFactory = new Factory("PolylineTool", "Polyline Editor",
        MapTool::factory, *Vrui::getToolManager());

    //requires one input device
    polylineFactory->setNumDevices(1);
    //requires two buttons on the first (and only) input device
    polylineFactory->setNumButtons(0,2);

    Vrui::getToolManager()->addClass(polylineFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);
}


const Vrui::ToolFactory* MapTool::
getFactory() const
{
    return factory;
}


END_CRUSTA
