#include <crusta/map/MapTool.h>

#include <cassert>

#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>

#include <crusta/vrui.h>


#include <crusta/StatsManager.h>


namespace crusta {


MapTool::Factory* MapTool::factory = NULL;

MapTool::
MapTool(const Vrui::ToolFactory* iFactory,
        const Vrui::ToolInputAssignment& inputAssignment) :
    Tool(iFactory, inputAssignment), toolId(MapManager::BAD_TOOLID),
    mode(MODE_IDLE), prevPosition(Math::Constants<Geometry::Point<double,3>::Scalar>::max)
{
}

MapTool::
~MapTool()
{
    MapManager* mapMan = crusta->getMapManager();

    Shape*& curShape = mapMan->getActiveShape(toolId);
    if (curShape != NULL)
    {
        unselectShape(curShape, curControl);
        mapMan->updateActiveShape(toolId);
    }

    mapMan->unregisterMappingTool(toolId);
}

Vrui::ToolFactory* MapTool::
init(Vrui::ToolFactory* parent)
{
    Factory* mapFactory = new Factory("MapTool", "Mapping Tool", parent,
                                      *Vrui::getToolManager());
    Vrui::getToolManager()->addClass(mapFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    MapTool::factory = mapFactory;

    return MapTool::factory;
}


void MapTool::
createShape(Shape*& shape, Shape::ControlId& control, const Geometry::Point<double,3>&)
{
    shape   = NULL;
    control = Shape::BAD_ID;
}
void MapTool::
deleteShape(Shape*& shape, Shape::ControlId& control)
{
    shape   = NULL;
    control = Shape::BAD_ID;
}
void MapTool::
addControlPoint(Shape*& shape, Shape::ControlId& control, const Geometry::Point<double,3>& pos)
{
    assert(shape != NULL);

    //if we have no valid control then we want to extend the line
    if (control == Shape::BAD_ID)
    {
        double distance;
        Shape::End end;
        shape->selectExtremity(pos, distance, end);

        control = shape->addControlPoint(pos, end);
    }
    else
    {
        switch (control.type)
        {
///\todo for now only support refinement
            case Shape::CONTROL_SEGMENT:
                control = shape->refine(control, pos);
                break;
            default:
                break;
        }
    }
}

void MapTool::
removeControl(Shape*& shape, Shape::ControlId& control)
{
    assert(shape != NULL);

    //the default implementation can only remove control points
    if (control!=Shape::BAD_ID && control.type==Shape::CONTROL_POINT)
        shape->removeControlPoint(control);
}

void MapTool::
unselectShape(Shape*& shape, Shape::ControlId& control)
{
    shape   = NULL;
    control = Shape::BAD_ID;
}


MapTool::ShapePtrs MapTool::
getShapes()
{
    return ShapePtrs();
}


Geometry::Point<double,3> MapTool::
getPosition()
{
    Vrui::NavTrackerState nav = Vrui::getDeviceTransformation(getButtonDevice(0));
    Vrui::NavTrackerState::Vector trans = nav.getTranslation();
    return Geometry::Point<double,3>(trans[0], trans[1], trans[2]);
}

void MapTool::
selectShape(const Geometry::Point<double,3>& pos)
{
    MapManager* mapMan = crusta->getMapManager();
    Shape*& curShape = mapMan->getActiveShape(toolId);
    Shape*  oldShape = curShape;

    ShapePtrs shapes = getShapes();

    double threshold = 1.0 / Vrui::getNavigationTransformation().getScaling();
    threshold       *= mapMan->getSelectDistance();

    bool noShapeSelected = true;
    for (ShapePtrs::iterator it=shapes.begin(); it!=shapes.end(); ++it)
    {
        double distance;
        Shape::ControlId control = (*it)->select(pos, distance);
        if (control!=Shape::BAD_ID && distance<=threshold)
        {
            curShape        = *it;
            threshold       = distance;
            noShapeSelected = false;
        }
    }

    if (noShapeSelected && curShape!=NULL)
        unselectShape(curShape, curControl);

    //inform the manager that the active shape has changed
    if (curShape != oldShape)
        mapMan->updateActiveShape(toolId);
}

void MapTool::
selectControl(const Geometry::Point<double,3>& pos)
{
    MapManager* mapMan = crusta->getMapManager();
    Shape*& curShape   = mapMan->getActiveShape(toolId);
    assert(curShape != NULL);

    curControl = Shape::BAD_ID;

    double distance;
    Shape::ControlId control = curShape->select(pos, distance,
                                         mapMan->getPointSelectionBias());

    if (control == Shape::BAD_ID)
        return;

    double threshold = 1.0 / Vrui::getNavigationTransformation().getScaling();
    threshold       *= mapMan->getSelectDistance();
    if (distance > threshold)
        return;

    curControl = control;
}

#if 0
void MapTool::
addPointAtEnds(const Geometry::Point<double,3>& pos)
{
    double distance;
    Shape::End end;
    Shape*& curShape = crusta->getMapManager()->getActiveShape(toolId);
    curShape->selectExtremity(pos, distance, end);

    curControl = curShape->addControlPoint(pos, end);
}
#endif



const Vrui::ToolFactory* MapTool::
getFactory() const
{
    return factory;
}


void MapTool::
frame()
{
if (PROJECTION_FAILED)
    return;

statsMan.start(StatsManager::EDITLINE);

    //handle motion
    Geometry::Point<double,3> pos = getPosition();
    pos = crusta->mapToUnscaledGlobe(pos);
    if (pos == prevPosition)
        return;

    switch (mode)
    {
        case MODE_DRAGGING:
        {
            MapManager* mapMan = crusta->getMapManager();
            Shape*& curShape   = mapMan->getActiveShape(toolId);
            assert(curShape != NULL);
///\todo implement a way to check validity of curControl that doesn't suck
            assert(curShape->isValid(curControl));

///\todo HACK: avoid moving control point if projection has failed Vis2010
            float scaleFac = Vrui::getNavigationTransformation().getScaling();
            if (scaleFac*Geometry::dist(pos, curControl.handle->pos) > 5.0)
                return;

            curShape->moveControlPoint(curControl, pos);
            break;
        }

        case MODE_SELECTING_CONTROL:
        {
            Shape*& curShape = crusta->getMapManager()->getActiveShape(toolId);
            if (curShape == NULL)
            {
                curControl = Shape::BAD_ID;
                mode       = MODE_IDLE;
            }
            else
            {
                selectControl(pos);
            }
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

statsMan.stop(StatsManager::EDITLINE);
}

void MapTool::
display(GLContextData& contextData) const
{
    Shape*& curShape = crusta->getMapManager()->getActiveShape(toolId);
    if (curShape==NULL || curShape->getControlPoints().size()<1)
        return;

    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    GLdouble depthRange[2];
    glGetDoublev(GL_DEPTH_RANGE, depthRange);

    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_POINT_BIT);

    glDisable(GL_LIGHTING);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glDepthRange(0.0, 0.0);

    //compute the centroids
    Geometry::Point<double,3> centroid(0);
    const Shape::ControlPointList& controlPoints = curShape->getControlPoints();
    std::vector<Geometry::Point<double,3> > cps;
    for (Shape::ControlPointList::const_iterator it=controlPoints.begin();
         it!=controlPoints.end(); ++it)
    {
        cps.push_back(crusta->mapToScaledGlobe(it->pos));
        const Geometry::Point<double,3>& cp = cps.back();
        for (int i=0; i<3; ++i)
            centroid[i] += cp[i];
    }
    int numPoints = static_cast<int>(cps.size());
    double norm = 1.0 / numPoints;
    for (int i=0; i<3; ++i)
        centroid[i] *= norm;

    glPushMatrix();

    Vrui::Vector centroidTranslation(centroid[0], centroid[1],
                                     centroid[2]);
    Vrui::NavTransform nav =
        Vrui::getDisplayState(contextData).modelviewNavigational;
    nav *= Vrui::NavTransform::translate(centroidTranslation);
    glLoadMatrix(nav);

    //draw the control points of the current shape
    glPointSize(4.0f);
    glColor3f(0.3f, 0.5f, 1.0f);
    glBegin(GL_POINTS);
    for (int i=0; i<numPoints; ++i)
    {
        glVertex3f(cps[i][0] - centroid[0],
                   cps[i][1] - centroid[1],
                   cps[i][2] - centroid[2]);
    }
    glEnd();

    //draw the current control element
    glLineWidth(5.0);
    glPointSize(6.0);

    if (curControl != Shape::BAD_ID)
    {
        switch (curControl.type)
        {
            case Shape::CONTROL_POINT:
            {
                Geometry::Point<double,3> p = curControl.handle->pos;
                p        = crusta->mapToScaledGlobe(p);

                glColor3f(0.3f, 0.9f, 0.5f);
                glBegin(GL_POINTS);
                glVertex3f(p[0]-centroid[0],p[1]-centroid[1],p[2]-centroid[2]);
                glEnd();

                break;
            }

            case Shape::CONTROL_SEGMENT:
            {
                Shape::ControlId si = curShape->previousControl(curControl);
                Geometry::Point<double,3> s            = si.handle->pos;
                s                   = crusta->mapToScaledGlobe(s);
                Shape::ControlId ei = curShape->nextControl(curControl);
                Geometry::Point<double,3> e            = ei.handle->pos;
                e                   = crusta->mapToScaledGlobe(e);

                glColor3f(0.3f, 0.9f, 0.5f);
                glBegin(GL_LINES);
                glVertex3f(s[0]-centroid[0],s[1]-centroid[1],s[2]-centroid[2]);
                glVertex3f(e[0]-centroid[0],e[1]-centroid[1],e[2]-centroid[2]);
                glEnd();

                break;
            }

            default:
                break;
        }
    }

    glPopMatrix();
    glPopAttrib();
    glDepthRange(depthRange[0], depthRange[1]);
    glActiveTexture(activeTexture);
}

void MapTool::
buttonCallback(int buttonSlotIndex,
               Vrui::InputDevice::ButtonCallbackData* cbData)
{
statsMan.start(StatsManager::EDITLINE);

    Geometry::Point<double,3> pos = getPosition();
    pos        = crusta->mapToUnscaledGlobe(pos);

    Shape*& curShape = crusta->getMapManager()->getActiveShape(toolId);

    if (cbData->newButtonState)
    {
        if (buttonSlotIndex==0)
        {
            switch (mode)
            {
                case MODE_IDLE:
                {
                    createShape(curShape, curControl, pos);
                    crusta->getMapManager()->updateActiveShape(toolId);
                    mode = MODE_DRAGGING;
                    break;
                }

                case MODE_SELECTING_CONTROL:
                {
                    selectControl(pos);
                    addControlPoint(curShape, curControl, pos);
                    mode = MODE_DRAGGING;
                    break;
                }

                case MODE_SELECTING_SHAPE:
                {
                    if (curShape != NULL)
                    {
                        deleteShape(curShape, curControl);
                    }
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
                    removeControl(curShape, curControl);
                    //destroy empty shapes
                    if (curShape->getControlPoints().size()==0)
                    {
                        deleteShape(curShape, curControl);
                        mode = MODE_IDLE;
                    }
                    else
                        mode = MODE_SELECTING_CONTROL;
                    break;
                }

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
        if (buttonSlotIndex==0)
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
                    {
                        mode = MODE_SELECTING_CONTROL;
                    }
                    else
                    {
                        mode = MODE_IDLE;
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }

statsMan.stop(StatsManager::EDITLINE);
}


void MapTool::
setupComponent(Crusta* nCrusta)
{
    CrustaComponent::setupComponent(nCrusta);
    toolId = crusta->getMapManager()->registerMappingTool();
}

} //namespace crusta
