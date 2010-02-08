#include <crusta/map/MapTool.h>

#include <cassert>
///\todo remove
#include <iostream>

#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLTransformationWrappers.h>
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
    Tool(iFactory, inputAssignment), mode(MODE_IDLE), curShape(NULL),
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

    double threshold = 1.0 / Vrui::getNavigationTransformation().getScaling();
    threshold       *= crusta->getMapManager()->getSelectDistance();

    for (ShapePtrs::iterator it=shapes.begin(); it!=shapes.end(); ++it)
    {
        double distance;
        Shape::Id control = (*it)->select(pos, distance);
        if (control!=Shape::BAD_ID && distance<=threshold)
        {
            curShape  = *it;
            threshold = distance;
        }
    }
}

void MapTool::
selectControl(const Point3& pos)
{
    curControl = Shape::BAD_ID;
    assert(curShape != NULL);

    MapManager* mapMan = crusta->getMapManager();
    double distance;
    Shape::Id control = curShape->select(pos, distance,
                                         mapMan->getPointSelectionBias());

    if (control == Shape::BAD_ID)
        return;

    double threshold = 1.0 / Vrui::getNavigationTransformation().getScaling();
    threshold       *= mapMan->getSelectDistance();
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
std::cout << "frame: MODE_DRAGGING -> MODE_SELECTING_CONTROL" << std::endl;
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
display(GLContextData& contextData) const
{
    if (curShape==NULL || curControl==Shape::BAD_ID)
        return;

    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);
    GLdouble depthRange[2];
    glGetDoublev(GL_DEPTH_RANGE, depthRange);

    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_POINT_BIT);
    glPushMatrix();
    glMultMatrix(Vrui::getNavigationTransformation());

    glDisable(GL_LIGHTING);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glDepthRange(0.0, 0.0);

    glLineWidth(3.0);
    glPointSize(4.0);

    switch (curControl.type)
    {
        case Shape::CONTROL_POINT:
        {
            const Point3& p = curShape->getControlPoint(curControl);

            glColor3f(0.9f, 0.6f, 0.3f);
            glBegin(GL_POINTS);
            glVertex3f(p[0], p[1], p[2]);
            glEnd();

            break;
        }

        case Shape::CONTROL_SEGMENT:
        {
            Shape::Id si    = curShape->previousControl(curControl);
            const Point3& s = curShape->getControlPoint(si);
            Shape::Id ei    = curShape->nextControl(curControl);
            const Point3& e = curShape->getControlPoint(ei);

            glColor3f(0.6f, 0.55f, 0.2f);
            glBegin(GL_LINES);
            glVertex3f(s[0], s[1], s[2]);
            glVertex3f(e[0], e[1], e[2]);
            glEnd();

            break;
        }

        default:
            break;
    }

    glPopMatrix();
    glPopAttrib();
    glDepthRange(depthRange[0], depthRange[1]);
    glActiveTexture(activeTexture);
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
std::cout << "MODE_IDLE -> MODE_DRAGGING" << std::endl;
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
std::cout << "MODE_SELECTING_CONTROL -> MODE_DRAGGING" << std::endl;
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
                case MODE_DRAGGING:
                {
                    assert(curControl!=Shape::BAD_ID &&
                           curControl.type==Shape::CONTROL_POINT);

                    curShape->removeControlPoint(curControl);
                    curControl = Shape::BAD_ID;
                    mode       = MODE_SELECTING_CONTROL;
                    break;
                }

                case MODE_IDLE:
                case MODE_SELECTING_CONTROL:
                {
                    selectShape(pos);
                    mode = MODE_SELECTING_SHAPE;
 std::cout << "MODE_IDLE,MODE_SELECTING_CONTROL -> MODE_SELECTING_SHAPE" << std::endl;
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
std::cout << "MODE_DRAGGING -> MODE_SELECTING_CONTROL" << std::endl;
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
                    {
                        mode = MODE_SELECTING_CONTROL;
std::cout << "MODE_SELECTING_SHAPE -> MODE_SELECTING_CONTROL" << std::endl;
                    }
                    else
                    {
                        mode = MODE_IDLE;
std::cout << "MODE_SELECTING_SHAPE -> MODE_IDLE" << std::endl;
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }
}



END_CRUSTA
