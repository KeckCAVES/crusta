#ifndef crusta_VerticalScaleDialog_h
#define crusta_VerticalScaleDialog_h

#include <crustacore/basics.h>
#include <crusta/glbasics.h>
#include <crusta/vrui.h>
#include <crusta/Dialog.h>
#include <crusta/Crusta.h>

namespace crusta {

class VerticalScaleDialog : public Dialog
{
public:
  VerticalScaleDialog();
protected:
  void init();
  void changeScaleCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
private:
  GLMotif::Label* scaleLabel;
};

}
#endif
