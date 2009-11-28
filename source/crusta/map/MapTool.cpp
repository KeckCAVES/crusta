#include <crusta/map/MapTool.h>

#include <cassert>

#include <Geometry/OrthogonalTransformation.h>
#include <Math/Constants.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>


BEGIN_CRUSTA


MapTool::Factory* MapTool::factory = NULL;

MapTool::
MapTool(const Vrui::ToolFactory* iFactory,
        const Vrui::ToolInputAssignment& inputAssignment) :
    Vrui::Tool(iFactory, inputAssignment), mode(MODE_IDLE), curShape(NULL),
    prevPosition(Math::Constants<Point3::Scalar>::max)
{
}

Vrui::ToolFactory* MapTool::
init(Vrui::ToolFactory* parent)
{
    Factory* mapFactory = new Factory("MapTool", "Mapping Tool", parent,
                                      *Vrui::getToolManager());
    Vrui::getToolManager()->addClass(mapFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    return mapFactory;
}


Shape* MapTool::
createShape()
{
    return NULL;
}
MapTool::ShapePtrs MapTool::
getShapes()
{
    return ShapePtrs();
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

    ShapePtrs shapes = getShapes();

    double distance = 1.0 / Vrui::getNavigationTransformation().getScaling();
    distance       *= Crusta::getMapManager()->getSelectDistance();

    for (ShapePtrs::iterator it=shapes.begin(); it!=shapes.end(); ++it)
    {
        double dist;
        Shape::Id control = (*it)->select(pos, dist);
        if (control!=Shape::BAD_ID && dist<=distance)
        {
            curShape = *it;
            distance = dist;
        }
    }
}

void MapTool::
selectControl(const Point3& pos)
{
    curControl = Shape::BAD_ID;

    assert(curShape != NULL);
    double distance;
    Shape::Id control = curShape->select(pos, distance);

    if (control == Shape::BAD_ID)
        return;

    double threshold = 1.0 / Vrui::getNavigationTransformation().getScaling();
    threshold       *= Crusta::getMapManager()->getSelectDistance();
    if (distance > threshold)
        return;

    curControl = control;
}

void MapTool::
addPointAtEnds(const Point3& pos)
{
    double distance;
    Shape::End end;
    curShape->selectExtremity(pos, distance, end);

    curControl = curShape->addControlPoint(pos, end);
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
    if (pos == prevPosition)
        return;

    switch (mode)
    {
        case MODE_DRAGGING:
        {
            assert(curShape != NULL);
            if (!curShape->moveControlPoint(curControl, pos))
            {
                curControl = Shape::BAD_ID;
                mode       = MODE_SELECTING_CONTROL;
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

        default:
            break;
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
                case MODE_IDLE:
                {
                    curShape   = createShape();
                    curControl = curShape->addControlPoint(pos);
                    mode       = MODE_DRAGGING;
                    break;
                }

                case MODE_SELECTING_CONTROL:
                {
                    selectControl(pos);
                    if (curControl==Shape::BAD_ID)
                        addPointAtEnds(pos);

                    if (curControl.type == Shape::CONTROL_SEGMENT)
                    {
                        curControl = curShape->refine(curControl, pos);
                        //make sure to update the rendering of the control hints
                    }
                    mode = MODE_DRAGGING;
                    break;
                }

                default:
                    break;
            }
        }
        else
        {
            switch (mode)
            {
                case MODE_IDLE:
                case MODE_SELECTING_CONTROL:
                {
                    selectShape(pos);
                    mode = MODE_SELECTING_SHAPE;
                    break;
                }

                default:
                    break;
            }
        }
    }
    else
    {
        if (buttonIndex == 0)
        {
            switch (mode)
            {
                case MODE_DRAGGING:
                {
                    mode = MODE_SELECTING_CONTROL;
                    break;
                }

                default:
                    break;
            }
        }
        else
        {
            switch (mode)
            {
                case MODE_SELECTING_SHAPE:
                {
                    if (curShape != NULL)
                        mode = MODE_SELECTING_CONTROL;
                    else
                        mode = MODE_IDLE;
                    break;
                }

                default:
                    break;
            }
        }
    }
}



END_CRUSTA
