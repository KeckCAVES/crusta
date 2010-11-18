#ifndef _MapManager_H_
#define _MapManager_H_

#include <map>
#include <string>

#include <GLMotif/Button.h>
#include <GLMotif/FileSelectionDialog.h>
#include <GLMotif/ListBox.h>
#include <GLMotif/ToggleButton.h>

#include <crusta/CrustaComponent.h>
#include <crusta/DataManager.h>
#include <crusta/map/PolylineRenderer.h>
#include <crusta/map/Shape.h>


class GLContextData;

namespace GLMotif {
    class Menu;
    class PopupWindow;
}
namespace Vrui {
    class ToolFactory;
}


BEGIN_CRUSTA


class Polyline;


class MapManager : public CrustaComponent
{
public:
    typedef std::vector<Polyline*> PolylinePtrs;


    MapManager(Vrui::ToolFactory* parentToolFactory, Crusta* iCrusta);
    ~MapManager();

    /** Destroy all the current map features */
    void deleteAllShapes();

    /** Load a new mapping dataset */
    void load(const char* filename);
    /** Save the current mapping dataset */
    void save(const char* filename, const char* format);

    /** Register a mapping tool with the manager to receive a registration id */
    int registerMappingTool();
    /** Alert the manager that registered tool is terminating */
    void unregisterMappingTool(int toolId);

    /** Enable tools to access the shape they can manipulate */
    Shape*& getActiveShape(int toolId);
    /** Notify the manager that the tool has activated another shape */
    void updateActiveShape(int toolId);

    const Shape::Symbol& getActiveSymbol();

    Scalar getSelectDistance() const;
    Scalar getPointSelectionBias() const;

    void addPolyline(Polyline* line);
    PolylinePtrs& getPolylines();
    void removePolyline(Polyline* line);

///\todo Vis2010 testing: update line coverage for given line sections
    void addShapeCoverage(Shape* shape,
                          const Shape::ControlPointHandle& startCP,
                          const Shape::ControlPointHandle& endCP);
    void removeShapeCoverage(Shape* shape,
                             const Shape::ControlPointHandle& startCP,
                             const Shape::ControlPointHandle& endCP);
    void inheritShapeCoverage(const NodeData& parent, NodeData& child);

    /** generate line data for the subset of render nodes that are outdated */
    void updateLineData(SurfaceApproximation& surface);

    void processVerticalScaleChange();

    void frame();
    void display(GLContextData& contextData,
                 const SurfaceApproximation& surface) const;

    void addMenuEntry(GLMotif::Menu* mainMenu);
    void openSymbolsGroupCallback(GLMotif::Button::SelectCallbackData* cbData);
    void symbolChangedCallback(
        GLMotif::ListBox::ItemSelectedCallbackData* cbData);
    void closeSymbolsGroupCallback(GLMotif::Button::SelectCallbackData* cbData);

    class ShapeCoverageManipulator : public Shape::IntersectionFunctor
    {
    public:
        void setShape(Shape* nShape);
        void setSegment(const Shape::ControlPointHandle& nSegment);
    protected:
        Shape*                    shape;
        Shape::ControlPointHandle segment;
    };

    class ShapeCoverageAdder : public ShapeCoverageManipulator
    {
    //- inherited from Shape::IntersectionFunctor
    public:
        virtual void operator()(NodeData& node, bool isLeaf);
    };

    class ShapeCoverageRemover : public ShapeCoverageManipulator
    {
    //- inherited from Shape::IntersectionFunctor
    public:
        virtual void operator()(NodeData& node, bool isLeaf);
    };

    static const int BAD_TOOLID = -1;

protected:
    typedef std::map<std::string, int>                   SymbolNameMap;
    typedef std::map<int,         std::string>           SymbolReverseNameMap;
    typedef std::map<int,         Shape::Symbol>         SymbolMap;
    typedef std::map<std::string, GLMotif::PopupWindow*> SymbolGroupMap;

    void produceMapControlDialog(GLMotif::Menu* mainMenu);
    void produceMapSymbolSubMenu(GLMotif::Menu* mainMenu);

    void showMapControlDialogCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void loadMapCallback(GLMotif::Button::SelectCallbackData* cbData);
    void saveMapCallback(GLMotif::Button::SelectCallbackData* cbData);
    void loadMapFileOKCallback(
        GLMotif::FileSelectionDialog::OKCallbackData* cbData);
    void loadMapFileCancelCallback(
        GLMotif::FileSelectionDialog::CancelCallbackData* cbData);

    Scalar selectDistance;
    Scalar pointSelectionBias;

///\todo enable support for multiple tools -> multiple activeShapes
    Shape*        activeShape;
    Shape::Symbol activeSymbol;

    IdGenerator32     polylineIds;
    PolylinePtrs      polylines;

    SymbolNameMap        symbolNameMap;
    SymbolReverseNameMap symbolReverseNameMap;
    SymbolMap            symbolMap;
    SymbolGroupMap       symbolGroupMap;

    PolylineRenderer polylineRenderer;

    GLMotif::PopupWindow* mapControlDialog;
    GLMotif::Label*       mapSymbolLabel;
    GLMotif::DropdownBox* mapOutputFormat;
};


END_CRUSTA


#endif //_MapManager_H_
