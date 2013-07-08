#ifndef crusta_Dialog_h
#define crusta_Dialog_h

#include <crustacore/basics.h>
#include <crusta/glbasics.h>
#include <crusta/vrui.h>

namespace crusta {

class Dialog
{
public:
  void createMenuEntry(GLMotif::Container* menu);

protected:
  virtual void init();
  void showCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
  void closeCallback(Misc::CallbackData* cbData);

  std::string name;
  std::string label;

  GLMotif::PopupWindow*  dialog;
  GLMotif::Container*    parentMenu;
  GLMotif::ToggleButton* toggle;
};

}
#endif
