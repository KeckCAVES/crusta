#ifndef _ElevationRangeShiftTool_H_
#define _ElevationRangeShiftTool_H_


#include <crusta/Tool.h>


BEGIN_CRUSTA


class ElevationRangeShiftTool : public Tool
{
    friend class Vrui::GenericToolFactory<ElevationRangeShiftTool>;

public:
    typedef Vrui::GenericToolFactory<ElevationRangeShiftTool> Factory;

    class ChangeCallbackData : public Misc::CallbackData
    {
    public:
        ElevationRangeShiftTool* tool;
        Scalar height;

        ChangeCallbackData(ElevationRangeShiftTool* iTool, Scalar iHeight);
    };

    ElevationRangeShiftTool(const Vrui::ToolFactory* iFactory,
                const Vrui::ToolInputAssignment& inputAssignment);
    virtual ~ElevationRangeShiftTool();

    Misc::CallbackList& getChangeCallbacks();

    static Vrui::ToolFactory* init(Vrui::ToolFactory* parent);

protected:
    Point3 getPosition();
    void notifyChange();

    bool isDragging;
    Point3 curPos;
    Misc::CallbackList changeCallbacks;

    static const Scalar markerSize;

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


#endif //_ElevationRangeShiftTool_H_
