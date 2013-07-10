#include <crusta/VerticalScaleDialog.h>

namespace crusta {

VerticalScaleDialog::VerticalScaleDialog():
  scaleLabel(NULL)
{
  name  = "VerticalScaleDialog";
  label = "Vertical Scale";
}

void VerticalScaleDialog::init()
{
  Dialog::init();

  const GLMotif::StyleSheet* style = Vrui::getWidgetManager()->getStyleSheet();

  GLMotif::RowColumn* root = new GLMotif::RowColumn("ScaleRoot", dialog, false);

  GLMotif::Slider* slider = new GLMotif::Slider(
    "ScaleSlider", root, GLMotif::Slider::HORIZONTAL,
    10.0 * style->fontHeight);
  slider->setValue(0.0);
  slider->setValueRange(-0.5, 2.5, 0.00001);
  slider->getValueChangedCallbacks().add(
    this,
    &VerticalScaleDialog::changeScaleCallback);

  scaleLabel = new GLMotif::Label("ScaleLabel", root, "1.0x");

  root->setNumMinorWidgets(2);
  root->manageChild();
}

void VerticalScaleDialog::changeScaleCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
  double newVerticalScale = pow(10, cbData->value);
  CRUSTA->setVerticalScale(newVerticalScale);

  std::ostringstream oss;
  oss.precision(2);
  oss << newVerticalScale << "x";
  scaleLabel->setString(oss.str().c_str());
}

}
