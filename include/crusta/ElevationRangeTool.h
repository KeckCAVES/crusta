#ifndef _ElevationRangeTool_H_
#define _ElevationRangeTool_H_


#include <crusta/Tool.h>


BEGIN_CRUSTA


class ElevationRangeTool : public Tool
{
    friend class Vrui::GenericToolFactory<ElevationRangeTool>;

public:
    typedef Vrui::GenericToolFactory<ElevationRangeTool> Factory;

    class ChangeCallback
    {
    public:
        virtual void operator()(Scalar min, Scalar max);
    };
    
    ElevationRangeTool(const Vrui::ToolFactory* iFactory,
                const Vrui::ToolInputAssignment& inputAssignment);
    virtual ~ElevationRangeTool();

    static Vrui::ToolFactory* init(Vrui::ToolFactory* parent);

protected:
    static Factory* factory;

    Point3 ends[2];

//- Inherited from Vrui::Tool
public:
    virtual void initialize();
    virtual const Vrui::ToolFactory* getFactory() const;

    virtual void frame();
    virtual void display(GLContextData& contextData) const;

    virtual void buttonCallback(int deviceIndex, int buttonIndex,
                                Vrui::InputDevice::ButtonCallbackData* cbData);
    
//- Inherited from Crusta::Component
public:
    virtual void setupComponent(Crusta* nCrusta);
};

END_CRUSTA


#endif //_ElevationRangeTool_H_
