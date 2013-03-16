#ifndef _Tool_H_
#define _Tool_H_


#include <crusta/CrustaComponent.h>

#include <crusta/vrui.h>


namespace crusta {


class Tool;

class Tool : public Vrui::Tool, public CrustaComponent
{
    friend class Vrui::GenericToolFactory<Tool>;

public:
    typedef Vrui::GenericToolFactory<Tool> Factory;

    Tool(const Vrui::ToolFactory* iFactory,
         const Vrui::ToolInputAssignment& inputAssignment);
    virtual ~Tool();

    static Vrui::ToolFactory* init(Vrui::ToolFactory* parent);

private:
    static Factory* factory;

//- Inherited from Vrui::Tool
public:
    virtual const Vrui::ToolFactory* getFactory() const;
};


} //namespace crusta


#endif //_Tool_H_
