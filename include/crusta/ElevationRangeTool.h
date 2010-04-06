#ifndef _ElevationRangeTool_H_
#define _ElevationRangeTool_H_


#include <crusta/Tool.h>


BEGIN_CRUSTA


class ElevationRangeTool : public Tool
{
    friend class Vrui::GenericToolFactory<ElevationRangeTool>;

public:
    typedef Vrui::GenericToolFactory<ElevationRangeTool> Factory;

    class ChangeCallbackData : public Misc::CallbackData
    {
    public:
        ElevationRangeTool* tool;
        Scalar min;
        Scalar max;

        ChangeCallbackData(ElevationRangeTool* iTool, Scalar iMin, Scalar iMax);
    };

    ElevationRangeTool(const Vrui::ToolFactory* iFactory,
                const Vrui::ToolInputAssignment& inputAssignment);
    virtual ~ElevationRangeTool();

    Misc::CallbackList& getChangeCallbacks();

    static Vrui::ToolFactory* init(Vrui::ToolFactory* parent);

protected:
    Point3 getPosition();
    void notifyChange();

    int controlPointsSet;
    int controlPointsHover;
    int controlPointsSelected;

    Point3 ends[2];

    Misc::CallbackList changeCallbacks;

    static const Scalar markerSize;
    static const Scalar selectDistance;

private:
    static Factory* factory;

//- Inherited from Vrui::Tool
public:
    virtual const Vrui::ToolFactory* getFactory() const;

    virtual void frame();
    virtual void display(GLContextData& contextData) const;

    virtual void buttonCallback(int deviceIndex, int buttonIndex,
                                Vrui::InputDevice::ButtonCallbackData* cbData);
};

END_CRUSTA


#endif //_ElevationRangeTool_H_
