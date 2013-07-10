#ifndef _CrustaApp_H_
#define _CrustaApp_H_

#include <crustacore/basics.h>
#include <crusta/glbasics.h>
#include <crusta/CrustaComponent.h>
#include <crusta/SurfaceProbeTool.h>
#include <crusta/Dialog.h>
#include <crusta/LightSettingsDialog.h>
#include <crusta/VerticalScaleDialog.h>
#include <crusta/OpacityDialog.h>
#include <crusta/TerrainColorSettingsDialog.h>
#include <crusta/LayerSettingsDialog.h>
#include <crusta/vrui.h>

namespace crusta {

class Crusta;

class CrustaApp : public Vrui::Application
{
public:
    CrustaApp(int& argc, char**& argv, char**& appDefaults);
    ~CrustaApp();

private:
    void handleArg(const std::string& arg);

private:
    void produceMainMenu();

    void produceDataDialog();
    void showDataDialogCallback(GLMotif::Button::SelectCallbackData* cbData);
    void addDataCallback(GLMotif::Button::SelectCallbackData* cbData);
    void addDataFileOkCallback(
        GLMotif::FileSelectionDialog::OKCallbackData* cbData);
    void addDataFileCancelCallback(
        GLMotif::FileSelectionDialog::CancelCallbackData* cbData);
    void removeDataCallback(GLMotif::Button::SelectCallbackData* cbData);
    void clearDataCallback(GLMotif::Button::SelectCallbackData* cbData);
    void loadDataOkCallback(GLMotif::Button::SelectCallbackData* cbData);
    void loadDataCancelCallback(GLMotif::Button::SelectCallbackData* cbData);

    void alignSurfaceFrame(Vrui::SurfaceNavigationTool::AlignmentData& alignmentData);

    void changeColorMapCallback(
        GLMotif::ColorMapEditor::ColorMapChangedCallbackData* cbData);
    void changeColorMapRangeCallback(
        GLMotif::RangeEditor::RangeChangedCallbackData* cbData);

    void showPaletteEditorCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);

    void decorateLinesCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);

    void debugGridCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void debugSpheresCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);

    void surfaceSamplePickedCallback(
        SurfaceProbeTool::SampleCallbackData* cbData);

    void resetNavigationCallback(Misc::CallbackData* cbData);

    GLMotif::PopupMenu*   popMenu;
    GLMotif::RadioBox*    curTool;

    GLMotif::PopupWindow*    dataDialog;
    GLMotif::ListBox*        dataListBox;
    std::vector<std::string> dataPaths;

    GLMotif::PaletteEditor* paletteEditor;

    VerticalScaleDialog        verticalScaleSettings;
    OpacityDialog              opacitySettings;
    LightSettingsDialog        lightSettings;
    TerrainColorSettingsDialog terrainColorSettings;
    LayerSettingsDialog        layerSettings;

    /** the crusta instance */
    Crusta* crusta;
		
//- inherited from Vrui::Application
public:
    virtual void frame();
    virtual void display(GLContextData& contextData) const;
    virtual void toolCreationCallback(
        Vrui::ToolManager::ToolCreationCallbackData* cbData);
    virtual void toolDestructionCallback(
        Vrui::ToolManager::ToolDestructionCallbackData* cbData);
};

} //namespace crusta

#endif //_CrustaApp_H_
