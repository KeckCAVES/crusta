#include <crusta/LightSettingsDialog.h>

namespace crusta {

LightSettingsDialog::LightSettingsDialog():
  viewerHeadlightStates(Vrui::getNumViewers()),
  enableSun(false),
  sun(Vrui::getLightsourceManager()->createLightsource(false)),
  sunAzimuth(180.0),
  sunElevation(45.0)
{
  name  = "LightSettingsDialog";
  label = "Light Settings";

  //save all viewers' headlight enable states
  for (int i=0; i<Vrui::getNumViewers(); ++i)
  {
    viewerHeadlightStates[i] =
      Vrui::getViewer(i)->getHeadlight().isEnabled();
  }

  //update the sun parameters
  updateSun();
}

LightSettingsDialog::~LightSettingsDialog()
{
  Vrui::getLightsourceManager()->destroyLightsource(sun);
}

void LightSettingsDialog::init()
{
  Dialog::init();

  const GLMotif::StyleSheet* style = Vrui::getWidgetManager()->getStyleSheet();
  GLMotif::RowColumn* lightSettings = new GLMotif::RowColumn("LightSettings", dialog, false);
  lightSettings->setNumMinorWidgets(2);

  // Create a toggle button and two sliders to manipulate the sun light source
  GLMotif::Margin* enableSunToggleMargin = new GLMotif::Margin("SunToggleMargin", lightSettings, false);
  enableSunToggleMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HFILL, GLMotif::Alignment::VCENTER));
  GLMotif::ToggleButton* enableSunToggle = new GLMotif::ToggleButton("SunToggle", enableSunToggleMargin, "Sun Light Source");
  enableSunToggle->setToggle(enableSun);
  enableSunToggle->getValueChangedCallbacks().add(this, &LightSettingsDialog::enableSunToggleCallback);
  enableSunToggleMargin->manageChild();

  GLMotif::RowColumn* sunBox = new GLMotif::RowColumn("SunBox", lightSettings, false);
  sunBox->setOrientation(GLMotif::RowColumn::VERTICAL);
  sunBox->setNumMinorWidgets(2);
  sunBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);

  sunAzimuthTextField = new GLMotif::TextField("SunAzimuthTextField", sunBox, 5);
  sunAzimuthTextField->setFloatFormat(GLMotif::TextField::FIXED);
  sunAzimuthTextField->setFieldWidth(3);
  sunAzimuthTextField->setPrecision(0);
  sunAzimuthTextField->setValue(double(sunAzimuth));

  sunAzimuthSlider = new GLMotif::Slider("SunAzimuthSlider", sunBox, GLMotif::Slider::HORIZONTAL,style->fontHeight*10.0f);
  sunAzimuthSlider->setValueRange(0.0,360.0,1.0);
  sunAzimuthSlider->setValue(double(sunAzimuth));
  sunAzimuthSlider->getValueChangedCallbacks().add(this, &LightSettingsDialog::sunAzimuthSliderCallback);

  sunElevationTextField = new GLMotif::TextField("SunElevationTextField", sunBox, 5);
  sunElevationTextField->setFloatFormat(GLMotif::TextField::FIXED);
  sunElevationTextField->setFieldWidth(2);
  sunElevationTextField->setPrecision(0);
  sunElevationTextField->setValue(double(sunElevation));

  sunElevationSlider = new GLMotif::Slider("SunElevationSlider", sunBox, GLMotif::Slider::HORIZONTAL,style->fontHeight*10.0f);
  sunElevationSlider->setValueRange(-90.0,90.0,1.0);
  sunElevationSlider->setValue(double(sunElevation));
  sunElevationSlider->getValueChangedCallbacks().add(this, &LightSettingsDialog::sunElevationSliderCallback);

  sunBox->manageChild();
  lightSettings->manageChild();
}

void LightSettingsDialog::updateSun()
{
  if(enableSun) sun->enable();
    else sun->disable();

  // Compute the light source's direction vector
  Vrui::Scalar z  = Math::sin(Math::rad(sunElevation));
  Vrui::Scalar xy = Math::cos(Math::rad(sunElevation));
  Vrui::Scalar x  = xy * Math::sin(Math::rad(sunAzimuth));
  Vrui::Scalar y  = xy * Math::cos(Math::rad(sunAzimuth));
  sun->getLight().position = GLLight::Position(GLLight::Scalar(x), GLLight::Scalar(y), GLLight::Scalar(z), GLLight::Scalar(0));
}

void LightSettingsDialog::enableSunToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
  // Set the sun enable flag
  enableSun = cbData->set;

  // Enable/disable all viewers' headlights
  if (enableSun) {
    for (int i=0; i<Vrui::getNumViewers(); ++i) Vrui::getViewer(i)->setHeadlightState(false);
  } else {
    for (int i=0; i<Vrui::getNumViewers(); ++i) Vrui::getViewer(i)->setHeadlightState(viewerHeadlightStates[i]);
  }

  // Update the sun light source
  updateSun();

  Vrui::requestUpdate();
}

void LightSettingsDialog::sunAzimuthSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
  // update the sun azimuth angle
  sunAzimuth = Vrui::Scalar(cbData->value);

  // update the sun azimuth value label
  sunAzimuthTextField->setValue(double(cbData->value));

  // update the sun light source
  updateSun();

  Vrui::requestUpdate();
}

void LightSettingsDialog::sunElevationSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
  // update the sun elevation angle
  sunElevation=Vrui::Scalar(cbData->value);

  // update the sun elevation value label
  sunElevationTextField->setValue(double(cbData->value));

  // update the sun light source
  updateSun();

  Vrui::requestUpdate();
}

}
