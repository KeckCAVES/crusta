#include <crusta/map/PolylineTool.h>

#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>
#include <crusta/map/Polyline.h>


BEGIN_CRUSTA


PolylineTool::Factory* PolylineTool::factory = NULL;


PolylineTool::
PolylineTool(const Vrui::ToolFactory* iFactory,
             const Vrui::ToolInputAssignment& inputAssignment) :
    MapTool(iFactory, inputAssignment)
{
}
PolylineTool::
~PolylineTool()
{
}

Vrui::ToolFactory* PolylineTool::
init(Vrui::ToolFactory* parent)
{
    Factory* polylineFactory = new Factory("PolylineTool",
        "Polyline Drawing Tool", parent, *Vrui::getToolManager());

    //one device is required
    polylineFactory->setNumDevices(1);
    //two buttons are required on the first (and only) device
    polylineFactory->setNumButtons(0, 2);

    Vrui::getToolManager()->addClass(polylineFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    return polylineFactory;
}


Shape* PolylineTool::
createShape()
{
    Polyline* newLine = new Polyline;
    Crusta::getMapManager()->addPolyline(newLine);

    return newLine;
}

PolylineTool::ShapePtrs PolylineTool::
getShapes()
{
    MapManager::PolylinePtrs& polylines =
        Crusta::getMapManager()->getPolylines();

    int numPolylines = static_cast<int>(polylines.size());
    ShapePtrs shapes;
    shapes.resize(numPolylines);
    for (int i=0; i<numPolylines; ++i)
        shapes[i] = polylines[i];

    return shapes;
}


const Vrui::ToolFactory* PolylineTool::
getFactory() const
{
    return factory;
}


END_CRUSTA
