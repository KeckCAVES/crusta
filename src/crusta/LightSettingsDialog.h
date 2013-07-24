#ifndef crusta_LightSettingsDialog_h
#define crusta_LightSettingsDialog_h

#include <crustacore/basics.h>
#include <crusta/glbasics.h>
#include <crusta/vrui.h>
#include <crusta/Dialog.h>

namespace crusta {

class LightSettingsDialog : public Dialog
{
public:
  LightSettingsDialog();
  ~LightSettingsDialog();

  void init();
  void enableSunToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
  void sunAzimuthSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
  void sunElevationSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);

  GLMotif::TextField* sunAzimuthTextField;
  GLMotif::Slider* sunAzimuthSlider;
  GLMotif::TextField* sunElevationTextField;
  GLMotif::Slider* sunElevationSlider;
};

}
#endif
