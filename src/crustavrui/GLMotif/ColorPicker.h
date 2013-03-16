#ifndef _ColorPicker_H_
#define _ColorPicker_H_


#include <vector>

#include <crustavrui/GLMotif/ColorHexagon.h>

#include <crustavrui/vrui.h>


namespace GLMotif {

class Label;

class ColorPicker : public RowColumn
{
public:
    /** type for colors with opacity values */
    typedef GLColor<GLfloat,4> Color;

    /** base class for callback data sent by color picker */
    class CallbackData : public Misc::CallbackData
    {
    public:
        /** pointer to the color map that sent the callback */
        ColorPicker* colorPicker;

        CallbackData(ColorPicker* iColorPicker);
    };

    class ColorChangedCallbackData : public CallbackData
    {
    public:
        ColorChangedCallbackData(ColorPicker* iColorPicker,
            const Color& iOldColor, const Color& iNewColor);

        /** previously selected color */
        Color oldColor;
        /** new selected color */
        Color newColor;
    };

    ColorPicker(const char* iName, Container* iParent, bool iManageChild=true);
    virtual ~ColorPicker();

    /** set the current color of the picker */
    void setCurrentColor(const Color& currentColor);

    /** returns list of color map change callbacks */
    Misc::CallbackList& getColorChangedCallbacks();

private:
    /** update the hexagon/slider representation of the current color */
    void updateWidgets();

    /** callback to process change in color selection from the hexagon */
    void colorpickCallback(ColorHexagon::ColorChangedCallbackData* cbData);
    /** callback to process change in the value slider */
    void valueCallback(Slider::ValueChangedCallbackData* cbData);
    /** callback to process change in the color sliders */
    void colorSliderCallback(Slider::ValueChangedCallbackData* cbData);

    /** the current color */
    Color color;
    /** the current value of the color */
    Scalar value;

    /** the color picking hexagon */
    ColorHexagon* hexagon;
    /** the color value slider */
    Slider* valueSlider;
    /** labels to show the current RGBA values */
    Label* rgbaLabels[4];
    /** sliders to directly modify the RGBA channels of the color */
    Slider* rgbaSliders[4];

    /** list of callbacks to be called when the color changes */
    Misc::CallbackList colorChangedCallbacks;
};


} //end namespace GLMotif


#endif //_ColorPicker_H_
