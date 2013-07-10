#include <cassert>
#include <crusta/TerrainColorSettingsDialog.h>
#include <crusta/CrustaSettings.h>

namespace crusta {

TerrainColorSettingsDialog::TerrainColorSettingsDialog():
  currentButton(NULL),
  colorPicker("TerrainColorSettingsColorPicker",
    Vrui::getWidgetManager(),
    "Pick Color")
{
  name  = "TerrainColorSettings";
  label = "Terrain Color Settings";
}

void TerrainColorSettingsDialog::init()
{
  Dialog::init();

  const GLMotif::StyleSheet* style = Vrui::getWidgetManager()->getStyleSheet();

  colorPicker.setCloseButton(true);
  colorPicker.getColorPicker()->getColorChangedCallbacks().add(this, &TerrainColorSettingsDialog::colorChangedCallback);

  GLMotif::RowColumn* root = new GLMotif::RowColumn("TCSRoot", dialog, false);
  root->setNumMinorWidgets(2);

  GLMotif::Button* button = NULL;

//- Emissive Color
  new GLMotif::Label("TCSEmissive", root, "Emissive:");
  button = new GLMotif::Button("TSCEmissiveButton", root, "");
  button->setBackgroundColor(SETTINGS->terrainEmissiveColor);
  button->getSelectCallbacks().add(this, &TerrainColorSettingsDialog::colorButtonCallback);

//- Ambient Color
  new GLMotif::Label("TCSAmbient", root, "Ambient:");
  button = new GLMotif::Button("TSCAmbientButton", root, "");
  button->setBackgroundColor(SETTINGS->terrainAmbientColor);
  button->getSelectCallbacks().add(this, &TerrainColorSettingsDialog::colorButtonCallback);

//- Diffuse Color
  new GLMotif::Label("TCSDiffuse", root, "Diffuse:");
  button = new GLMotif::Button("TSCDiffuseButton", root, "");
  button->setBackgroundColor(SETTINGS->terrainDiffuseColor);
  button->getSelectCallbacks().add(this, &TerrainColorSettingsDialog::colorButtonCallback);

//- Specular Color
  new GLMotif::Label("TCSSpecular", root, "Specular:");
  button = new GLMotif::Button("TSCSpecularButton", root, "");
  button->setBackgroundColor(SETTINGS->terrainSpecularColor);
  button->getSelectCallbacks().add(this, &TerrainColorSettingsDialog::colorButtonCallback);

//- Specular Shininess
  new GLMotif::Label("TCSShininess", root, "Shininess:");
  GLMotif::RowColumn* shininessRoot = new GLMotif::RowColumn("TCSShininessRoot", root, false);
  shininessRoot->setNumMinorWidgets(2);

  GLMotif::Slider* slider = new GLMotif::Slider(
    "TCSShininessSlider",
    shininessRoot,
    GLMotif::Slider::HORIZONTAL,
    5.0 * style->fontHeight);
  slider->setValue(SETTINGS->terrainShininess);
  slider->setValueRange(0.0f, 128.0f, 1.0f);
  slider->getValueChangedCallbacks().add(this, &TerrainColorSettingsDialog::shininessChangedCallback);
  shininessField = new GLMotif::TextField(
    "SSShininessField",
    shininessRoot,
    3);
  shininessField->setPrecision(3);
  shininessField->setValue(SETTINGS->terrainShininess);

  shininessRoot->manageChild();

  root->manageChild();
}

void TerrainColorSettingsDialog::colorButtonCallback(GLMotif::Button::SelectCallbackData* cbData)
{
  GLMotif::WidgetManager* manager = Vrui::getWidgetManager();

  //hide the picker if user clicks on the same button when the picker is up
  if (manager->isVisible(&colorPicker) && cbData->button==currentButton)
  {
    Vrui::popdownPrimaryWidget(&colorPicker);
    return;
  }

  //update the current button
  currentButton = cbData->button;

  //update the color of the picker
  if (strcmp(currentButton->getName(), "TSCEmissiveButton") == 0) {
    colorPicker.setTitleString("Pick Emissive Color");
    colorPicker.getColorPicker()->setCurrentColor(SETTINGS->terrainEmissiveColor);
  } else if (strcmp(currentButton->getName(), "TSCAmbientButton") == 0) {
    colorPicker.setTitleString("Pick Ambient Color");
    colorPicker.getColorPicker()->setCurrentColor(SETTINGS->terrainAmbientColor);
  } else if (strcmp(currentButton->getName(), "TSCDiffuseButton") == 0) {
    colorPicker.setTitleString("Pick Diffuse Color");
    colorPicker.getColorPicker()->setCurrentColor(SETTINGS->terrainDiffuseColor);
  } else if (strcmp(currentButton->getName(), "TSCSpecularButton") == 0) {
    colorPicker.setTitleString("Pick Specular Color");
    colorPicker.getColorPicker()->setCurrentColor(SETTINGS->terrainSpecularColor);
  }

  //pop up the picker if necessary
  if (!manager->isVisible(&colorPicker))
  {
    manager->popupPrimaryWidget(&colorPicker, manager->calcWidgetTransformation(cbData->button));
  }
}

void TerrainColorSettingsDialog::colorChangedCallback(GLMotif::ColorPicker::ColorChangedCallbackData* cbData)
{
  assert(currentButton != NULL);

  //update the color of the corresponding button
  currentButton->setBackgroundColor(cbData->newColor);
  //update appropriate color setting
  if (strcmp(currentButton->getName(), "TSCEmissiveButton") == 0)
    SETTINGS->terrainEmissiveColor = cbData->newColor;
  else if (strcmp(currentButton->getName(), "TSCAmbientButton") == 0)
    SETTINGS->terrainAmbientColor = cbData->newColor;
  else if (strcmp(currentButton->getName(), "TSCDiffuseButton") == 0)
    SETTINGS->terrainDiffuseColor = cbData->newColor;
  else if (strcmp(currentButton->getName(), "TSCSpecularButton") == 0)
    SETTINGS->terrainSpecularColor = cbData->newColor;
}

void TerrainColorSettingsDialog::shininessChangedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
  //update the shininess setting and corresponding text field
  SETTINGS->terrainShininess = cbData->value;
  shininessField->setValue(cbData->value);
}

}
