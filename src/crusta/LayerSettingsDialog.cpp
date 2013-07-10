#include <crusta/LayerSettingsDialog.h>

#include <crustavrui/GLMotif/PaletteEditor.h>
#include <crusta/ColorMapper.h>

namespace crusta {

static const Geometry::Point<int,2> DATALISTBOX_SIZE(30, 8);

LayerSettingsDialog::LayerSettingsDialog(GLMotif::PaletteEditor* editor):
  listBox(NULL),
  paletteEditor(editor),
  buttonRoot(NULL)
{
  name  = "LayerSettings";
  label = "Layer Settings";
}

void LayerSettingsDialog::init()
{
  Dialog::init();

  GLMotif::RowColumn* root = new GLMotif::RowColumn("LayerRoot", dialog, false);

  GLMotif::ScrolledListBox* box = new GLMotif::ScrolledListBox(
    "LayerListBox",
    root,
    GLMotif::ListBox::ALWAYS_ONE,
    DATALISTBOX_SIZE[0],
    DATALISTBOX_SIZE[1]);
  listBox = box->getListBox();
  listBox->setAutoResize(true);
  listBox->getValueChangedCallbacks().add(this, &LayerSettingsDialog::layerChangedCallback);

  buttonRoot = new GLMotif::RowColumn("LayerButtonRoot", root);
  buttonRoot->setNumMinorWidgets(2);
  buttonRoot->setPacking(GLMotif::RowColumn::PACK_GRID);

  updateLayerList();

  root->manageChild();
}

void LayerSettingsDialog::updateLayerList()
{
  listBox->clear();

  ColorMapper::Strings layerNames = COLORMAPPER->getLayerNames();
  for (ColorMapper::Strings::iterator it=layerNames.begin(); it!=layerNames.end(); ++it) {
    listBox->addItem(it->c_str());
  }

  if (COLORMAPPER->getHeightColorMapIndex() != -1)
    listBox->selectItem(COLORMAPPER->getHeightColorMapIndex());
  else if (!layerNames.empty())
    listBox->selectItem(0);
  //make sure the buttonRoot doesn't remain empty
  else
     new GLMotif::Margin("LayerEmptyMargin", buttonRoot);
}

void LayerSettingsDialog::layerChangedCallback(GLMotif::ListBox::ValueChangedCallbackData* cbData)
{
  //remove all the buttons
  buttonRoot->removeWidgets(0);

  COLORMAPPER->setActiveLayer(cbData->newSelectedItem);
  if (cbData->newSelectedItem >= 0) {
    GLMotif::ToggleButton* visibleButton = new GLMotif::ToggleButton(
      "LayerVisibleButton",
      buttonRoot,
      "Visible");
    bool visible = COLORMAPPER->isVisible(cbData->newSelectedItem);
    visibleButton->setToggle(visible);
    visibleButton->getValueChangedCallbacks().add(this, &LayerSettingsDialog::visibleCallback);
  }

  if (COLORMAPPER->isFloatLayer(cbData->newSelectedItem)) {
    GLMotif::ToggleButton* clampButton = new GLMotif::ToggleButton(
      "LayerClampButton",
      buttonRoot,
      "Clamp");
    bool clamped = COLORMAPPER->isClamped(cbData->newSelectedItem);
    clampButton->setToggle(clamped);
    clampButton->getValueChangedCallbacks().add(this, &LayerSettingsDialog::clampCallback);

    Misc::ColorMap& colorMap = COLORMAPPER->getColorMap(cbData->newSelectedItem);

    paletteEditor->getColorMapEditor()->getColorMap() = colorMap;
    paletteEditor->getColorMapEditor()->touch();
  }
}

void LayerSettingsDialog::visibleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
  int activeLayer = COLORMAPPER->getActiveLayer();
  COLORMAPPER->setVisible(activeLayer, cbData->set);
}

void LayerSettingsDialog::clampCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
  int activeLayer = COLORMAPPER->getActiveLayer();
  COLORMAPPER->setClamping(activeLayer, cbData->set);
}

}
