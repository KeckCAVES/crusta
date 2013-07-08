#include <crusta/Dialog.h>

namespace crusta {

void Dialog::createMenuEntry(GLMotif::Container* menu)
{
  init();
  parentMenu = menu;
  toggle = new GLMotif::ToggleButton((name+"Toggle").c_str(), parentMenu, label.c_str());
  toggle->setToggle(false);
  toggle->getValueChangedCallbacks().add(this, &Dialog::showCallback);
}

void Dialog::init()
{
  dialog = new GLMotif::PopupWindow((name+"Dialog").c_str(), Vrui::getWidgetManager(), label.c_str());
  dialog->setCloseButton(true);
  dialog->getCloseCallbacks().add(this, &Dialog::closeCallback);
}

void Dialog::showCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
  if(cbData->set) {
    Vrui::popupPrimaryWidget(dialog);
  } else {
    Vrui::popdownPrimaryWidget(dialog);
  }
}

void Dialog::closeCallback(Misc::CallbackData* cbData)
{
  toggle->setToggle(false);
}

}
