#ifndef _Tool_H_
#define _Tool_H_


#include <Vrui/Tools/GenericToolFactory.h>

#include <crusta/basics.h>


BEGIN_CRUSTA


class Tool;

class Tool : public Vrui::Tool
{
    friend class Vrui::GenericToolFactory<Tool>;

public:
    typedef Vrui::GenericToolFactory<Tool> Factory;

    Tool(const Vrui::ToolFactory* iFactory,
         const Vrui::ToolInputAssignment& inputAssignment);
    virtual ~Tool();

    static void init();

protected:
    static Factory* factory;

//- Inherited from Vrui::Tool
public:
    virtual const Vrui::ToolFactory* getFactory() const;
};


END_CRUSTA


#endif //_Tool_H_
