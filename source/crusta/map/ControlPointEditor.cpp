#include <crusta/map/ControlPointEditor.h>

#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Vrui.h>

#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>
#include <crusta/map/Shape.h>

BEGIN_CRUSTA

ControlPointEditor::
ControlPointEditor(Vrui::LocatorTool* locator) :
    LocatorToolAdapter(locator), dragging(false), curPoint(Shape::BAD_ID)
{
}

void ControlPointEditor::
motionCallback(Vrui::LocatorTool::MotionCallbackData* cbData)
{
    Vector3 trans = cbData->currentTransformation.getTranslation();
    Point3 pos(trans[0], trans[1], trans[2]);
    Shape* shape  = Crusta::getMapManager()->getSelectedShape();

    if (dragging)
    {
        if (shape==NULL || !shape->moveControlPoint(curPoint, pos))
        {
            curPoint = Shape::BAD_ID;
            dragging = false;
            return;
        }
    }
    else
    {
        if (shape != NULL)
        {
            //make sure to update the rendering of the control hints
        }
    }
}

void ControlPointEditor::
buttonPressCallback(Vrui::LocatorTool::ButtonPressCallbackData* cbData)
{
    MapManager* mapMan = Crusta::getMapManager();
    Shape* shape       = mapMan->getSelectedShape();
    Vector3 trans      = cbData->currentTransformation.getTranslation();
    Point3 pos(trans[0], trans[1], trans[2]);

    if (shape == NULL)
    {
        shape    = mapMan->createShape();
        curPoint = shape->addControlPoint(pos);
    }
    else
    {
        double distance;
        Shape::Id control = shape->select(pos, distance);
        if (control == Shape::BAD_ID)
            return;

        double threshold = 1.0/Vrui::getNavigationTransformation().getScaling();
        threshold       *= mapMan->getSelectDistance();

        if (distance > threshold)
            return;

        switch (control.type)
        {
            case Shape::CONTROL_POINT:
                curPoint = control;
                break;

            case Shape::CONTROL_SEGMENT:
                curPoint = shape->refine(control, pos);
                //make sure to update the rendering of the control hints
                break;

            default:
                return;
        }

    }

    dragging = true;
}

void ControlPointEditor::
buttonReleaseCallback(Vrui::LocatorTool::ButtonReleaseCallbackData* cbData)
{
    dragging = false;
}


END_CRUSTA
