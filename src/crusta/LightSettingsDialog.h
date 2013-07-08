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

protected:
  void init();
  void updateSun(void);
  void enableSunToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
  void sunAzimuthSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
  void sunElevationSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);

private:
  /** Initial enable states of all viewers' headlights */
  std::vector<bool> viewerHeadlightStates;
  /** Flag to store the sun activation state */
  bool enableSun;
  /** Light source representing the sun */
  Vrui::Lightsource* sun;
  /** Azimuth of sunlight direction */
  Vrui::Scalar sunAzimuth;
  /** Elevation of sunlight direction */
  Vrui::Scalar sunElevation;

  GLMotif::TextField* sunAzimuthTextField;
  GLMotif::Slider* sunAzimuthSlider;
  GLMotif::TextField* sunElevationTextField;
  GLMotif::Slider* sunElevationSlider;
};

}
#endif
