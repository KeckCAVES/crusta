#include <crustavrui/GLMotif/ColorPicker.h>

#include <cstring>
#include <iostream>
#include <sstream>

#include <crustavrui/vrui.h>

namespace GLMotif {

static const GLfloat COLORPICKER_MIN_VALUE = 0.0001;

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
    RowColumn(iName, iParent, false),
    color(1.0, 1.0, 1.0, 1.0), value(1.0)
{
///\todo allow changing the preferred size through API
const Vector preferredSize(6.0f, 6.0f, 0.0f);

    const GLMotif::StyleSheet* style =
        Vrui::getWidgetManager()->getStyleSheet();

    setNumMinorWidgets(2);

    RowColumn* hexaRoot = new RowColumn("pickerHexaRoot", this, false);
    hexaRoot->setNumMinorWidgets(1);
    hexagon = new ColorHexagon("pickerHexagon", hexaRoot, true);
    hexagon->setValue(1.0);
    hexagon->setPreferredSize(Vector(style->fontHeight*preferredSize[0],
                                     style->fontHeight*preferredSize[1],
                                     0.0f));
    hexagon->getColorChangedCallbacks().add(this,
                                            &ColorPicker::colorpickCallback);
    valueSlider = new Slider("pickerHexaSlider", hexaRoot,
                             Slider::HORIZONTAL,
                             preferredSize[0]*style->fontHeight);
    valueSlider->setValue(1.0);
    valueSlider->setValueRange(COLORPICKER_MIN_VALUE, 1.0, 0.01);
    valueSlider->getValueChangedCallbacks().add(this,
        &ColorPicker::valueCallback);

    hexaRoot->manageChild();

    RowColumn* rgbaRoot = new RowColumn("pickerRgbaRoot", this, false);
    rgbaRoot->setNumMinorWidgets(2);
    rgbaRoot->setOrientation(RowColumn::HORIZONTAL);

    static const char* names[4][2] = { {"pickerALabel", "pickerA"},
                                       {"pickerRLabel", "pickerR"},
                                       {"pickerGLabel", "pickerG"},
                                       {"pickerBLabel", "pickerB"} };
    static const GLMotif::Color colors[3] = {GLMotif::Color(1.0f, 0.0f, 0.0f),
                                             GLMotif::Color(0.0f, 1.0f, 0.0f),
                                             GLMotif::Color(0.0f, 0.0f, 1.0f)};
    for (int i=0; i<4; ++i)
    {
        rgbaLabels[i]  = new Label(names[i][0], rgbaRoot, "-");
        rgbaSliders[i] = new Slider(names[i][1], rgbaRoot, Slider::VERTICAL,
                                    0.5f*preferredSize[0]*style->fontHeight);
        if (i==0)
        {
            rgbaSliders[i]->setValueRange(0.0, 1.0, 0.001);
        }
        else
        {
            rgbaSliders[i]->setSliderColor(colors[i-1]);
            rgbaSliders[i]->setValueRange(COLORPICKER_MIN_VALUE, 1.0, 0.001);
        }
        rgbaSliders[i]->getValueChangedCallbacks().add(this,
            &ColorPicker::colorSliderCallback);
    }

    setCurrentColor(Color(1.0f, 1.0f, 1.0f, 1.0f));

    rgbaRoot->manageChild();

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
    //record color
    for (int i=0; i<3; ++i)
    {
        color[i] = currentColor[i]<COLORPICKER_MIN_VALUE ? COLORPICKER_MIN_VALUE
                                                         : currentColor[i];
    }
    color[3] = currentColor[3];

    //compute value
    GLfloat newValue = std::max(color[0], color[1]);
    newValue         = std::max(newValue, color[2]);
    value            = newValue;

    //make sure the change is visually propagated
    updateWidgets();
}

Misc::CallbackList& ColorPicker::
getColorChangedCallbacks()
{
    return colorChangedCallbacks;
}

void ColorPicker::
updateWidgets()
{
    //update the color hexagon and value slider
    hexagon->setValue(value);
    valueSlider->setValue(value);

    //update the rgba sliders
    static const int index[4] = {3, 0, 1, 2};
    for (int i=0; i<4; ++i)
    {
        std::ostringstream oss;
        oss.width(3);
        oss << (int)(color[index[i]]*255.0f + 0.5f);
        rgbaLabels[i]->setString(oss.str().c_str());
        rgbaSliders[i]->setValue(color[index[i]]);
    }
}


void ColorPicker::
colorpickCallback(ColorHexagon::ColorChangedCallbackData* cbData)
{
    //update the internal representation
    Color oldColor = color;
    for (int i=0; i<3; ++i)
        color[i] = cbData->color[i];

    updateWidgets();

    //forward the change from the hexagon
    ColorChangedCallbackData cb(this, oldColor, color);
    colorChangedCallbacks.call(&cb);
}

void ColorPicker::
valueCallback(Slider::ValueChangedCallbackData* cbData)
{
    //update the internal representation
    Color  oldColor = color;
    Scalar newValue = std::max(0.00001f, cbData->value);
    Scalar rescale  = newValue / value;
    for (int i=0; i<3; ++i)
        color[i] = rescale * color[i];
    value = newValue;

    updateWidgets();

    //forward the change from the hexagon
    ColorChangedCallbackData cb(this, oldColor, color);
    colorChangedCallbacks.call(&cb);
}

void ColorPicker::
colorSliderCallback(Slider::ValueChangedCallbackData* cbData)
{
    //update the internal representation
    Color oldColor = color;
    if (strcmp(cbData->slider->getName(), "pickerA") == 0)
        color[3] = cbData->value;
    else if (strcmp(cbData->slider->getName(), "pickerR") == 0)
        color[0] = cbData->value;
    else if (strcmp(cbData->slider->getName(), "pickerG") == 0)
        color[1] = cbData->value;
    else if (strcmp(cbData->slider->getName(), "pickerB") == 0)
        color[2] = cbData->value;

    GLfloat newValue = std::max(color[0], color[1]);
    newValue         = std::max(newValue, color[2]);
    value            = newValue;

    updateWidgets();

    //forward the change from the hexagon
    ColorChangedCallbackData cb(this, oldColor, color);
    colorChangedCallbacks.call(&cb);
}


} //end namespace GLMotif
