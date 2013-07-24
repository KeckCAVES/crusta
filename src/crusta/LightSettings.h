#ifndef crusta_LightSettings_h
#define crusta_LightSettings_h

#include <crustacore/basics.h>
#include <crusta/glbasics.h>
#include <crusta/vrui.h>
#include <crusta/Dialog.h>

namespace crusta {

class LightSettings
{
public:
  LightSettings();
  ~LightSettings();

  void updateSun(void);

  std::vector<bool> viewerHeadlightStates;
  bool enableSun;
  Vrui::Lightsource* sun;
  Vrui::Scalar sunAzimuth;
  Vrui::Scalar sunElevation;
};

}
#endif

