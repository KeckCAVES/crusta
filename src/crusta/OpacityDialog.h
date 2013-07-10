#ifndef crusta_OpacityDialog_h
#define crusta_OpacityDialog_h

#include <crustacore/basics.h>
#include <crusta/glbasics.h>
#include <crusta/vrui.h>
#include <crusta/Dialog.h>
#include <crusta/Crusta.h>

namespace crusta {

class OpacityDialog : public Dialog
{
public:
  OpacityDialog();
protected:
  void init();
  void changeOpacityCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
private:
  GLMotif::Label* opacityLabel;
};

}
#endif
