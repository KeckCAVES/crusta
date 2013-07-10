#ifndef crusta_LayerSettingsDialog_h
#define crusta_LayerSettingsDialog_h

#include <crustacore/basics.h>
#include <crusta/glbasics.h>
#include <crusta/vrui.h>
#include <crusta/Dialog.h>

namespace GLMotif {
  class PaletteEditor;
}

namespace crusta {

class LayerSettingsDialog : public Dialog
{
public:
  LayerSettingsDialog(GLMotif::PaletteEditor* editor);
  void updateLayerList();
protected:
  void init();
private:
  void layerChangedCallback(GLMotif::ListBox::ValueChangedCallbackData* cbData);
  void visibleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
  void clampCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);

  GLMotif::ListBox* listBox;
  GLMotif::PaletteEditor* paletteEditor;
  GLMotif::RowColumn* buttonRoot;
};

}
#endif
