#ifndef _GLMotif_ColorMapEditor_H_
#define _GLMotif_ColorMapEditor_H_


#include <crustavrui/Misc/ColorMap.h>

#include <crustavrui/vrui.h>


namespace GLMotif {

class ColorMapEditor : public Widget
{
public:
    /** base class for callback data sent by color maps */
    class CallbackData : public Misc::CallbackData
    {
    public:
        CallbackData(ColorMapEditor* colorMapWidget);
        ColorMapEditor* editor;
    };

    class SelectedControlPointChangedCallbackData : public CallbackData
    {
    public:
        enum Reason
        {
            SELECT,
            SWITCH,
            DESELECT
        };

        SelectedControlPointChangedCallbackData(ColorMapEditor* colorMapWidget,
            int oldControlPoint, int newControlPoint);

        int oldControlPoint;
        int newControlPoint;
        Reason reason;
    };

    class ColorMapChangedCallbackData:public CallbackData
    {
    public:
        ColorMapChangedCallbackData(ColorMapEditor* colorMapWidget);
    };


    ColorMapEditor(const char* name, Container* parent, bool manageChild=true);

    /** changes the margin width */
    void setMarginWidth(GLfloat newMarginWidth);
    /** sets a new preferred size */
    void setPreferredSize(const Vector& newPreferredSize);

    /** sets a new size for control points */
    void setControlPointSize(GLfloat newControlPointSize);
    /** sets the color for the selected control points */
    void setSelectedControlPointColor(
        const Color& newSelectedControlPointColor);

    /** selects the control point of the given index */
    void selectControlPoint(int controlPointIndex);
    /** deletes the selected intermediate control point (first and last cannot
        be deleted) */
    void deleteSelectedControlPoint();
    /** returns true if a control point is currently selected */
    bool hasSelectedControlPoint() const;

    /** inserts a new control point by interpolating the current color map */
    int insertControlPoint(double value, bool propagate=true);

    /** get a handle to the selected control point */
    Misc::ColorMap::Point& getSelectedControlPoint();
    /** get a handle to the managed color map */
    Misc::ColorMap& getColorMap();

    /** indicate a change to either the selected control point or the managed
        color map directly */
    void touch();

    /** returns list of selected control point change callbacks */
    Misc::CallbackList& getSelectedControlPointChangedCallbacks();
    /** returns list of color map change callbacks */
    Misc::CallbackList& getColorMapChangedCallbacks();

protected:
    typedef Misc::ColorMap::Points::iterator PointHandle;
    typedef Point                            ControlPoint;
    typedef std::vector<ControlPoint>        ControlPoints;

    /** updates the widget positions of all control points */
    void updateControlPoints(bool propagate=true);

    /** width of margin around color map area */
    GLfloat marginWidth;
    /** the color map area's preferred size */
    Vector preferredSize;
    /** position and size of color map area */
    Box colorMapAreaBox;

    /** size of control points */
    GLfloat controlPointSize;
    /** color to use for the selected control point */
    Color selectedControlPointColor;

    /** handle to currently selected control point */
    int selected;

    /** the control points */
    ControlPoints controlPoints;
    /** the managed color map */
    Misc::ColorMap colorMap;

    /** flag whether a control point is being dragged */
    bool isDragging;
    /** offset between pointer and dragged control point in widget
        coordinates */
    Point::Vector dragOffset;

    /** callbacks to be called when the selected control point changes */
    Misc::CallbackList selectedControlPointChangedCallbacks;
    /** callbacks to be called when the color map changes */
    Misc::CallbackList colorMapChangedCallbacks;

//- inherited from Widget
public:
    virtual Vector calcNaturalSize() const;
    virtual void resize(const Box& newExterior);
    virtual void draw(GLContextData& contextData) const;
    virtual bool findRecipient(Event& event);
    virtual void pointerButtonDown(Event& event);
    virtual void pointerButtonUp(Event& event);
    virtual void pointerMotion(Event& event);
};


} //end namespace GLMotif


#endif //_GLMotif_ColorMapEditor_H_
