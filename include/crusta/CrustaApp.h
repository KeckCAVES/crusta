#ifndef _CrustaApp_H_
#define _CrustaApp_H_


#include <GLMotif/Button.h>
#include <GLMotif/ColorMapEditor.h>
#include <GLMotif/ColorPickerWindow.h>
#include <GLMotif/FileAndFolderSelectionDialog.h>
#include <GLMotif/RangeEditor.h>
#include <GLMotif/Slider.h>
#include <GLMotif/ToggleButton.h>

#include <Vrui/SurfaceNavigationTool.h>
#include <Vrui/Application.h>
#include <Vrui/Geometry.h>

#include <crusta/basics.h>
#include <crusta/CrustaComponent.h>
#include <crusta/SurfaceProbeTool.h>


class GLContextData;

namespace GLMotif {
    class DropdownBox;
    class Label;
    class ListBox;
    class Menu;
    class PaletteEditor;
    class PopupMenu;
    class PopupWindow;
    class RadioBox;
    class RowColumn;
    class TextField;
}
namespace Vrui {
class Lightsource;
}


BEGIN_CRUSTA


class Crusta;

class CrustaApp : public Vrui::Application
{
public:
    CrustaApp(int& argc, char**& argv, char**& appDefaults);
    ~CrustaApp();

private:
    class Dialog
    {
    public:
        void createMenuEntry(GLMotif::Container* menu);

    protected:
        virtual void init();
        void showCallback(
            GLMotif::ToggleButton::ValueChangedCallbackData* cbData);

        std::string name;
        std::string label;

        GLMotif::PopupWindow* dialog;
        GLMotif::Container*   parentMenu;
    };

    class SpecularSettingsDialog : public Dialog, public CrustaComponent
    {
    public:
        SpecularSettingsDialog();
    protected:
        void init();
    private:
        void colorButtonCallback(
            GLMotif::Button::SelectCallbackData* cbData);
        void colorChangedCallback(
            GLMotif::ColorPicker::ColorChangedCallbackData* cbData);
        void shininessChangedCallback(
            GLMotif::Slider::ValueChangedCallbackData* cbData);

        GLMotif::ColorPickerWindow colorPicker;
        GLMotif::Button*           colorButton;
        GLMotif::TextField*        shininessField;
    };

    class LayerSettingsDialog : public Dialog
    {
    public:
        LayerSettingsDialog(GLMotif::PaletteEditor* editor);
        void updateLayerList();
    protected:
        void init();
    private:
        void layerChangedCallback(
            GLMotif::ListBox::ValueChangedCallbackData* cbData);
        void visibleCallback(
            GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
        void clampCallback(
            GLMotif::ToggleButton::ValueChangedCallbackData* cbData);

        GLMotif::ListBox*       listBox;
        GLMotif::PaletteEditor* paletteEditor;
        GLMotif::RowColumn*     buttonRoot;
    };

    void produceMainMenu();

    void produceDataDialog();
    void showDataDialogCallback(GLMotif::Button::SelectCallbackData* cbData);
    void addDataCallback(GLMotif::Button::SelectCallbackData* cbData);
    void addDataFileOkCallback(
        GLMotif::FileAndFolderSelectionDialog::OKCallbackData* cbData);
    void addDataFileCancelCallback(
        GLMotif::FileAndFolderSelectionDialog::CancelCallbackData* cbData);
    void removeDataCallback(GLMotif::Button::SelectCallbackData* cbData);
    void clearDataCallback(GLMotif::Button::SelectCallbackData* cbData);
    void loadDataOkCallback(GLMotif::Button::SelectCallbackData* cbData);
    void loadDataCancelCallback(GLMotif::Button::SelectCallbackData* cbData);

    void produceVerticalScaleDialog();
    void produceLightingDialog();

    void alignSurfaceFrame(const Vrui::SurfaceNavigationTool::AlignmentData& alignmentData);

    void changeColorMapCallback(
        GLMotif::ColorMapEditor::ColorMapChangedCallbackData* cbData);
    void changeColorMapRangeCallback(
        GLMotif::RangeEditor::RangeChangedCallbackData* cbData);

    void showVerticalScaleCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void changeScaleCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);

    void showLightingDialogCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void updateSun(void);
    void enableSunToggleCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void sunAzimuthSliderCallback(
        GLMotif::Slider::ValueChangedCallbackData* cbData);
    void sunElevationSliderCallback(
        GLMotif::Slider::ValueChangedCallbackData* cbData);

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

    double newVerticalScale;

    GLMotif::PopupMenu*   popMenu;
    GLMotif::RadioBox*    curTool;

    GLMotif::PopupWindow*    dataDialog;
    GLMotif::ListBox*        dataListBox;
    std::vector<std::string> dataPaths;

    GLMotif::PopupWindow* verticalScaleDialog;
    GLMotif::Label*       verticalScaleLabel;

    GLMotif::PopupWindow* lightingDialog;
    bool enableSun; // Flag to toggle sun lightsource
    bool* viewerHeadlightStates; // Initial enable states of all viewers' headlights
    Vrui::Lightsource* sun; // Light source representing the sun
    Vrui::Scalar sunAzimuth; // Azimuth of sunlight direction
    Vrui::Scalar sunElevation; // Elevation of sunlight direction
    GLMotif::TextField* sunAzimuthTextField;
    GLMotif::Slider* sunAzimuthSlider;
    GLMotif::TextField* sunElevationTextField;
    GLMotif::Slider* sunElevationSlider;

    GLMotif::PaletteEditor* paletteEditor;

    SpecularSettingsDialog specularSettings;
    LayerSettingsDialog layerSettings;

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

END_CRUSTA

#endif //_CrustaApp_H_
