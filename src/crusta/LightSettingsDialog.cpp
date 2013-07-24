#include <crusta/LightSettingsDialog.h>
#include <crusta/Crusta.h>

namespace crusta {

LightSettingsDialog::LightSettingsDialog()
{
  name  = "LightSettingsDialog";
  label = "Light Settings";
}

LightSettingsDialog::~LightSettingsDialog()
{
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
  enableSunToggle->setToggle(CRUSTA->getLightSettings()->enableSun);
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
  sunAzimuthTextField->setValue(double(CRUSTA->getLightSettings()->sunAzimuth));

  sunAzimuthSlider = new GLMotif::Slider("SunAzimuthSlider", sunBox, GLMotif::Slider::HORIZONTAL,style->fontHeight*10.0f);
  sunAzimuthSlider->setValueRange(0.0,360.0,1.0);
  sunAzimuthSlider->setValue(double(CRUSTA->getLightSettings()->sunAzimuth));
  sunAzimuthSlider->getValueChangedCallbacks().add(this, &LightSettingsDialog::sunAzimuthSliderCallback);

  sunElevationTextField = new GLMotif::TextField("SunElevationTextField", sunBox, 5);
  sunElevationTextField->setFloatFormat(GLMotif::TextField::FIXED);
  sunElevationTextField->setFieldWidth(2);
  sunElevationTextField->setPrecision(0);
  sunElevationTextField->setValue(double(CRUSTA->getLightSettings()->sunElevation));

  sunElevationSlider = new GLMotif::Slider("SunElevationSlider", sunBox, GLMotif::Slider::HORIZONTAL,style->fontHeight*10.0f);
  sunElevationSlider->setValueRange(-90.0,90.0,1.0);
  sunElevationSlider->setValue(double(CRUSTA->getLightSettings()->sunElevation));
  sunElevationSlider->getValueChangedCallbacks().add(this, &LightSettingsDialog::sunElevationSliderCallback);

  sunBox->manageChild();
  lightSettings->manageChild();
}

void LightSettingsDialog::enableSunToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
  // Set the sun enable flag
  CRUSTA->getLightSettings()->enableSun = cbData->set;

  // Update the sun light source
  CRUSTA->getLightSettings()->updateSun();

  Vrui::requestUpdate();
}

void LightSettingsDialog::sunAzimuthSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
  // update the sun azimuth angle
  CRUSTA->getLightSettings()->sunAzimuth = Vrui::Scalar(cbData->value);

  // update the sun azimuth value label
  sunAzimuthTextField->setValue(double(cbData->value));

  // update the sun light source
  CRUSTA->getLightSettings()->updateSun();

  Vrui::requestUpdate();
}

void LightSettingsDialog::sunElevationSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
  // update the sun elevation angle
  CRUSTA->getLightSettings()->sunElevation=Vrui::Scalar(cbData->value);

  // update the sun elevation value label
  sunElevationTextField->setValue(double(cbData->value));

  // update the sun light source
  CRUSTA->getLightSettings()->updateSun();

  Vrui::requestUpdate();
}

}
