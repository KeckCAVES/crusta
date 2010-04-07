#include <GLMotif/ColorHexagon.h>

#include <GLMotif/Container.h>
#include <GLMotif/Event.h>

#define LEFT_X  0.21132487
#define RIGHT_X 0.78867513

namespace GLMotif {

const ColorHexagon::Color baseColors[6] = {
    Color(1,0,0), Color(1,1,0), Color(0,1,0),
    Color(0,1,1), Color(0,0,1), Color(1,0,1) };

ColorHexagon::CallbackData::
CallbackData(ColorHexagon* iHexagon) :
hexagon(iHexagon)
{
}

ColorHexagon::ColorChangedCallbackData::
ColorChangedCallbackData(ColorHexagon* iHexagon,
                         const Color& iOldColor, const Color& iNewColor) :
CallbackData(iHexagon), oldColor(iOldColor), newColor(iNewColor)
{
}

ColorHexagon::
ColorHexagon(const char* iName, Container* iParent, bool iManageChild) :
    Widget(iName, iParent, false),
    marginWidth(0.0), preferredSize(0.0, 0.0, 0.0), isDragging(false)
{

    updateHexagon();

    if (iManageChild)
        manageChild();
}

ColorHexagon::
~ColorHexagon()
{
}

void ColorHexagon::
setValue(Scalar newValue)
{
    value = newValue;
    updateHexagon();
}

void ColorHexagon::
setMarginWidth(GLfloat newMarginWidth)
{
    /* Set the new margin width: */
    marginWidth = newMarginWidth;

    if (isManaged)
    {
        /* Try adjusting the widget size to accomodate the new margin width: */
        parent->requestResize(this, calcNaturalSize());
    }
    else
    {
        resize(Box(Vector(0.0f,0.0f,0.0f), calcNaturalSize()));
    }
}

void ColorHexagon::
setPreferredSize(const Vector& newPreferredSize)
{
    /* Set the new preferred size: */
    preferredSize = newPreferredSize;

    if (isManaged)
    {
        /* Try adjusting the widget size to accomodate the new preferred size:*/
        parent->requestResize(this, calcNaturalSize());
    }
    else
    {
        resize(Box(Vector(0.0f,0.0f,0.0f), calcNaturalSize()));
    }
}


Misc::CallbackList& ColorHexagon::
getColorChangedCallbacks()
{
    return colorChangedCallbacks;
}


void ColorHexagon::
updateHexagon()
{
    Vector minBox(hexagonAreaBox.origin);
    Vector maxBox(hexagonAreaBox.origin);
    maxBox[0] += hexagonAreaBox.size[0];
    maxBox[1] += hexagonAreaBox.size[1];
}


Vector ColorHexagon::
calcNaturalSize() const
{
    /* Return size of color hexagon area plus margin: */
    Vector result = preferredSize;
    result[0] += 2.0 * marginWidth;
    result[1] += 2.0 * marginWidth;

    return calcExteriorSize(result);
}

void ColorHexagon::
resize(const Box& newExterior)
{
    /* Resize the parent class widget: */
    Widget::resize(newExterior);

    /* Adjust the color hexagon area position: */
    hexagonAreaBox = getInterior();
    hexagonAreaBox.doInset(Vector(marginWidth,marginWidth,0.0f));

    /* Update the color hexagon area positions of all control points: */
    updateHexagon();
}

void ColorHexagon::
draw(GLContextData& contextData) const
{
}

bool ColorHexagon::
findRecipient(Event& event)
{
    if(isDragging)
    {
        /* This event belongs to me! */
        return event.setTargetWidget(this, event.calcWidgetPoint(this));
    }
    else
    {
        return Widget::findRecipient(event);
    }
}

void ColorHexagon::
pointerButtonDown(Event& event)
{
}

void ColorHexagon::
pointerButtonUp(Event& event)
{
}

void ColorHexagon::
pointerMotion(Event& event)
{
}

} //end namespace GLMotif

