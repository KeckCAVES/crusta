#ifndef crusta_TerrainColorSettingsDialog_h
#define crusta_TerrainColorSettingsDialog_h

#include <crustacore/basics.h>
#include <crustavrui/GLMotif/ColorMapEditor.h>
#include <crustavrui/GLMotif/ColorPickerWindow.h>
#include <crustavrui/GLMotif/RangeEditor.h>
#include <crusta/glbasics.h>
#include <crusta/vrui.h>
#include <crusta/Dialog.h>

namespace crusta {

class TerrainColorSettingsDialog : public Dialog
{
public:
  TerrainColorSettingsDialog();
protected:
  void init();
private:
  void colorButtonCallback(GLMotif::Button::SelectCallbackData* cbData);
  void colorChangedCallback(GLMotif::ColorPicker::ColorChangedCallbackData* cbData);
  void shininessChangedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);

  GLMotif::Button* currentButton;
  GLMotif::ColorPickerWindow colorPicker;
  GLMotif::TextField* shininessField;
};

}
#endif
