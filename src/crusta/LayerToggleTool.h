#ifndef _LayerToggleTool_H_
#define _LayerToggleTool_H_

#include <crusta/vrui.h>

namespace crusta {

class LayerToggleTool: public Vrui::Tool
{
  friend class Vrui::GenericToolFactory<LayerToggleTool>;
  public:
    typedef Vrui::GenericToolFactory<LayerToggleTool> Factory;
    LayerToggleTool(const Vrui::ToolFactory* factory, const Vrui::ToolInputAssignment& inputAssignment);
    ~LayerToggleTool();
    const Vrui::ToolFactory* getFactory() const;
    void buttonCallback(int buttonSlotIndex, Vrui::InputDevice::ButtonCallbackData* cbData);
    static Vrui::ToolFactory* init();
  private:
    bool state;
    static Factory* factory;
};

} //namespace crusta

#endif //_LayerToggleTool_H_
