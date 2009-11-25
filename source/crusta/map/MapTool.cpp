#include <crusta/map/MapTool.h>

BEGIN_CRUSTA

MapTool::
MapTool(const Vrui::ToolFactory* iFactory,
        const Vrui::ToolInputAssignment& inputAssignment) :
    Vrui::Tool(iFactory, inputAssignment), mode(MODE_IDLE), curShape(NULL),
    prevPosition(Math::Constants<Point3::Scalar>::max)
{
}

void MapTool::
init()
{
    assert(Tool::factory != NULL);
    Factory* mapFactory = new Factory("MapTool", "Abstract Map Tool",
                                      Tool::factory, Vrui::getToolManager());
    Vrui::getToolManager()->addClass(mapFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);
}


Point3 MapTool::
getPosition()
{
    Vrui::NavTrackerState nav = Vrui::getDeviceTransformation(getDevice(0));
    Vrui::NavTrackerState::Vector trans = nav.getTranslation();
    return Point3(trans[0], trans[1], trans[2]);
}

void MapTool::
selectShape(const Point3& pos)
{
    curShape   = NULL;
    curControl = Shape::BAD_ID;

    MapManager* mapMan = Crusta::getMapManager();
    const MapManager::PolylinePtrs& polylines = mapMan->getPolylines();
    
    double distance = 1.0 / Vrui::getNavigationTransformation().getScaling();
    distance       *= mapMan()->getSelectDistance();
    
    for (MapManager::PolylinePtrs::const_iterator it=polylines.begin();
         it!=polylines.end(); ++it)
    {
        double dist;
        Shape::Id control = it->select(pos, dist);
        if (control!=Shape::BAD_ID && dist<=distance)
        {
            curShape = *it;
            distance = dist;
        }
    }
}

void MapTool::
selectControl(const Point& pos)
{
    curControl = Shape::BAD_ID;

    assert(curShape != NULL);
    double distance;
    ShapeParam::Id control = curShape->select(pos, distance);
    
    if (control == ShapeParam::BAD_ID)
        return;
    
    double threshold = 1.0 / Vrui::getNavigationTransformation().getScaling();
    threshold       *= Crusta::getMapManager()->getSelectDistance();
    if (distance > threshold)
        return;
    
    curControl = control;
}



const Vrui::ToolFactory* MapTool::
getFactory() const
{
    return factory;
}


void MapTool::
frame()
{
    //handle motion
    Point3 pos = getPosition();
    if (pos == prePosition)
        return;
    
    switch (mode)
    {
        case MODE_DRAGGING:
        {
            assert(curShape != NULL);
            if (!curShape->moveControlPoint(curPoint, pos))
            {
                curPoint = Shape::BAD_ID;
                mode     = MODE_SELECTING_CONTROL;
            }
            break;
        }
            
        case MODE_SELECTING_CONTROL:
        {
            selectControl(pos);
            break;
        }
        
        case MODE_SELECTING_SHAPE:
        {
            selectShape(pos);
            break;
        }
    }
}

void MapTool::
buttonCallback(int deviceIndex, int buttonIndex,
               Vrui::InputDevice::ButtonCallbackData* cbData)
{
    Point3 pos = getPosition();

    if (cbData->newButtonState)
    {
        if (buttonIndex == 0)
        {
            switch (mode)
            {
                case MODE_SELECTING_CONTROL:
                {
                    selectControl(pos);
                    if (curControl != Shape::BAD_ID)
            }
        }
        else
        {
            
        }
    }
    else
    {
        
    }
}



END_CRUSTA
