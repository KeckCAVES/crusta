#include <crusta/LightSettings.h>

namespace crusta {

LightSettings::LightSettings():
  viewerHeadlightStates(Vrui::getNumViewers()),
  enableSun(false),
  sun(Vrui::getLightsourceManager()->createLightsource(false)),
  sunAzimuth(180.0),
  sunElevation(45.0)
{
  //save all viewers' headlight enable states
  for (int i=0; i<Vrui::getNumViewers(); ++i)
  {
    viewerHeadlightStates[i] = Vrui::getViewer(i)->getHeadlight().isEnabled();
  }

  //update the sun parameters
  updateSun();
}

LightSettings::~LightSettings()
{
  Vrui::getLightsourceManager()->destroyLightsource(sun);
}

void LightSettings::updateSun()
{
  if(enableSun) {
    sun->enable();
    for (int i=0; i<Vrui::getNumViewers(); ++i) Vrui::getViewer(i)->setHeadlightState(false);
  } else {
    sun->disable();
    for (int i=0; i<Vrui::getNumViewers(); ++i) Vrui::getViewer(i)->setHeadlightState(viewerHeadlightStates[i]);
  }

  // Compute the light source's direction vector
  Vrui::Scalar z  = Math::sin(Math::rad(sunElevation));
  Vrui::Scalar xy = Math::cos(Math::rad(sunElevation));
  Vrui::Scalar x  = xy * Math::sin(Math::rad(sunAzimuth));
  Vrui::Scalar y  = xy * Math::cos(Math::rad(sunAzimuth));
  sun->getLight().position = GLLight::Position(GLLight::Scalar(x), GLLight::Scalar(y), GLLight::Scalar(z), GLLight::Scalar(0));
}

}
