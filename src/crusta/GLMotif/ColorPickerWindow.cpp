#include <crusta/GLMotif/ColorPickerWindow.h>


namespace GLMotif {


ColorPickerWindow::
ColorPickerWindow(const char* iName, WidgetManager* iManager,
                  const char* iTitleString) :
    PopupWindow(iName, iManager, iTitleString)
{
    picker = new ColorPicker("ColorPicker", this);
}

ColorPicker* ColorPickerWindow::
getColorPicker()
{
    return picker;
}


} //end namespace GLMotif
