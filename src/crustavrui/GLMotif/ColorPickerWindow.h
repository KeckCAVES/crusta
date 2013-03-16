#ifndef _ColorPickerWindow_H_


#include <crustavrui/GLMotif/ColorPicker.h>

#include <crustavrui/vrui.h>


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
