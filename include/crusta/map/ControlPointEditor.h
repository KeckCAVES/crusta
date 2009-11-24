#ifndef _ControlPointEditor_H_
#define _ControlPointEditor_H_

#include <Vrui/LocatorToolAdapter.h>

#include <crusta/basics.h>
#include <crusta/Shape.h>


BEGIN_CRUSTA


/**
    The control point editor is a vrui tool that manipulates the location of
    the control points for the active shapes.
*/
class ControlPointEditor : public Vrui::LocatorToolAdapter
{
public:
    ControlPointEditor(Vrui::LocatorTool* locator);

protected:
    bool      dragging;
    Shape::Id curPoint;

//- Inherited from LocatorToolAdapter
public:
	void motionCallback(Vrui::LocatorTool::MotionCallbackData* cbData);
	void buttonPressCallback(
        Vrui::LocatorTool::ButtonPressCallbackData* cbData);
	void buttonReleaseCallback(
        Vrui::LocatorTool::ButtonReleaseCallbackData* cbData);
};

END_CRUSTA


#endif //_ControlPointEditor_H_
