#include <crusta/OpacityDialog.h>

namespace crusta {

OpacityDialog::OpacityDialog():
  opacityLabel(NULL)
{
  name  = "OpacityDialog";
  label = "Opacity";
}

void OpacityDialog::init()
{
  Dialog::init();

  const GLMotif::StyleSheet* style = Vrui::getWidgetManager()->getStyleSheet();

  GLMotif::RowColumn* root = new GLMotif::RowColumn("OpacityRoot", dialog, false);

  GLMotif::Slider* slider = new GLMotif::Slider(
    "OpacitySlider",
    root,
    GLMotif::Slider::HORIZONTAL,
    10.0 * style->fontHeight);
  slider->setValue(1.0);
  slider->setValueRange(0.0, 1.0, 0.01);
  slider->getValueChangedCallbacks().add(
    this,
    &OpacityDialog::changeOpacityCallback);

  opacityLabel = new GLMotif::Label("OpacityLabel", root, "1.0");

  root->setNumMinorWidgets(2);
  root->manageChild();
}

void OpacityDialog::changeOpacityCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
  double newOpacity = cbData->value;
  CRUSTA->setOpacity(newOpacity);

  std::ostringstream oss;
  oss.precision(2);
  oss << newOpacity;
  opacityLabel->setString(oss.str().c_str());
}

}
