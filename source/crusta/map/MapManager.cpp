#include <MapManager.h>

#include <crusta/ControlPointEditor.h>
#include <crusta/Polyline.h>

BEGIN_CRUSTA


MapManager::
MapManager() :
    selectDistance(0.001), selectedShape(NULL)
{}

MapManager::
~MapManager()
{
    for (ShapesPtrs::iterator it=shapes.begin(); it!=shapes.end(); ++it)
        delete *it;
}

double MapManager::
getSelectDistance() const
{
    return selectDistance;
}

Shape* MapManager::
getSelectedShape() const
{
    return selectShape;
}

Shape* MapManager::
createShape()
{
///\todo decide what to create based on the active map
    Shape* newShape = new Polyline;
    shapes.push_back(newShape);
    polylines.push_back(newShape);
    return newShape;
}


void MapManager::
frame()
{
}

void MapManager::
display(GLContextData& contextData) const
{
    //go through all the simple polylines and draw them

}


void MapManager::
createMapTool(MapTool toolId, Vrui::LocatorTool* locator)
{
    switch (toolId)
    {
        case MAPTOOL_CONTROLPOINTEDITOR:
        {
            ControlPointEditor* cpe      = new ControlPointEditor(locator);
            controlPointEditors[locator] = cpe;
            break;
        }

        default:
            return;
    }
}

void MapManager::
destroyMapTool(Vrui::LocatorTool* locator)
{
    controlPointEditors.erase(locator);
}

END_CRUSTA
