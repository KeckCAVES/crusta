#ifndef _ColorPickerWindow_H_


#include <crusta/GLMotif/ColorPicker.h>
#include <GLMotif/PopupWindow.h>


namespace GLMotif {


class ColorPickerWindow : public PopupWindow
{
public:
    ColorPickerWindow(const char* iName, WidgetManager* iManager,
                      const char* iTitleString);

    ColorPicker* getColorPicker();

private:
    ColorPicker* picker;
};


} //end namespace GLMotif


#endif //_ColorPickerWindow_H_
