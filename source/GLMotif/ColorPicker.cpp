#include <GLMotif/ColorPicker.h>

namespace GLMotif {

ColorPicker::CallbackData::
CallbackData(ColorPicker* iColorPicker) :
    colorPicker(iColorPicker)
{
}

ColorPicker::ColorChangedCallbackData::
ColorChangedCallbackData(ColorPicker* iColorPicker,
                         const Color& iOldColor, const Color& iNewColor) :
    CallbackData(iColorPicker), oldColor(iOldColor), newColor(iNewColor)
{
}

ColorPicker::
ColorPicker(const char* iName, Container* iParent, bool iManageChild) :
    Widget(iName, iParent, false),
    color(1.0, 1.0, 1.0, 1.0), value(1.0),
    marginWidth(0.0), preferredSize(0.0, 0.0, 0.0), isDragging(false)
{


    if (iManageChild)
        manageChild();
}

ColorPicker::
~ColorPicker()
{
}

void ColorPicker::
setCurrentColor(const Color& currentColor)
{
}

void ColorPicker::
setMarginWidth(GLfloat newMarginWidth)
{
}

void ColorPicker::
setPreferredSize(const Vector& newPreferredSize)
{
}


Misc::CallbackList& ColorPicker::
getColorChangedCallbacks()
{
    return colorChangedCallbacks;
}


Vector ColorPicker::
calcNaturalSize() const
{
    return Vector();
}

void ColorPicker::
resize(const Box& newExterior)
{
}

void ColorPicker::
draw(GLContextData& contextData) const
{
}

bool ColorPicker::
findRecipient(Event& event)
{
    return false;
}

void ColorPicker::
pointerButtonDown(Event& event)
{
}

void ColorPicker::
pointerButtonUp(Event& event)
{
}

void ColorPicker::
pointerMotion(Event& event)
{
}

} //end namespace GLMotif

