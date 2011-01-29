#ifndef _DebugTool_H_
#define _DebugTool_H_

#include <Vrui/Tools/GenericToolFactory.h>

#include <crusta/CrustaComponent.h>


BEGIN_CRUSTA


class DebugTool : public Vrui::Tool, public CrustaComponent
{
    friend class Vrui::GenericToolFactory<DebugTool>;

public:
    typedef Vrui::GenericToolFactory<DebugTool> Factory;

    DebugTool(const Vrui::ToolFactory* iFactory,
              const Vrui::ToolInputAssignment& inputAssignment);
    virtual ~DebugTool();

    static void init();

    int   getNumButtons();
    bool  getButton(int i);
    bool& getButtonLast(int i);

private:
    static Factory* factory;

    static const int numButtons = 1;

    bool buttons[numButtons];
    bool buttonsLast[numButtons];

    static bool bogusButton;

//- Inherited from Vrui::Tool
public:
    virtual const Vrui::ToolFactory* getFactory() const;

    virtual void frame();

    virtual void buttonCallback(int deviceIndex, int deviceButtonIndex,
                                Vrui::InputDevice::ButtonCallbackData* cbData);
};


END_CRUSTA


#endif //_DebugTool_H_
