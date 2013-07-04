#ifndef _SGToggleTool_H_
#define _SGToggleTool_H_

#include <crusta/vrui.h>

namespace crusta {

class SGToggleTool: public Vrui::Tool
{
  friend class Vrui::GenericToolFactory<SGToggleTool>;
  public:
    typedef Vrui::GenericToolFactory<SGToggleTool> Factory;
    SGToggleTool(const Vrui::ToolFactory* factory, const Vrui::ToolInputAssignment& inputAssignment);
    ~SGToggleTool();
    const Vrui::ToolFactory* getFactory() const;
    void buttonCallback(int buttonSlotIndex, Vrui::InputDevice::ButtonCallbackData* cbData);
    static Vrui::ToolFactory* init();
  private:
    static Factory* factory;
};

} //namespace crusta

#endif //_SGToggleTool_H_
