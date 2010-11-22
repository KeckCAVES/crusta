#ifndef _CrustaApp_H_
#define _CrustaApp_H_


#include <GLMotif/Button.h>
#include <GLMotif/ColorMap.h>
#include <GLMotif/ColorPickerWindow.h>
#include <GLMotif/FileAndFolderSelectionDialog.h>
#include <GLMotif/Slider.h>
#include <GLMotif/ToggleButton.h>

#include <Vrui/Application.h>
#include <Vrui/Geometry.h>

#include <crusta/basics.h>
#include <crusta/CrustaComponent.h>


class GLContextData;
class PaletteEditor;

namespace GLMotif {
    class DropdownBox;
    class Label;
    class Menu;
    class PopupMenu;
    class PopupWindow;
    class RadioBox;
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

    void produceMainMenu();

    void produceDataDialog();
    void showDataDialogCallback(GLMotif::Button::SelectCallbackData* cbData);
    void loadDemCallback(GLMotif::Button::SelectCallbackData* cbData);
    void loadDemFileOkCallback(
        GLMotif::FileAndFolderSelectionDialog::OKCallbackData* cbData);
    void loadDemFileCancelCallback(
        GLMotif::FileAndFolderSelectionDialog::CancelCallbackData* cbData);
    void loadColorCallback(GLMotif::Button::SelectCallbackData* cbData);
    void loadColorFileOkCallback(
        GLMotif::FileAndFolderSelectionDialog::OKCallbackData* cbData);
    void loadColorFileCancelCallback(
        GLMotif::FileAndFolderSelectionDialog::CancelCallbackData* cbData);
    void loadDataOkCallback(GLMotif::Button::SelectCallbackData* cbData);
    void loadDataCancelCallback(GLMotif::Button::SelectCallbackData* cbData);

    void produceTexturingSubmenu(GLMotif::Menu* mainMenu);
    void produceVerticalScaleDialog();
    void produceLightingDialog();

    void alignSurfaceFrame(Vrui::NavTransform& surfaceFrame);

    void changeTexturingModeCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void changeColorMapCallback(
        GLMotif::ColorMap::ColorMapChangedCallbackData* cbData);

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

    void resetNavigationCallback(Misc::CallbackData* cbData);

    double newVerticalScale;

    GLMotif::PopupMenu*   popMenu;
    GLMotif::RadioBox*    curTool;

    GLMotif::PopupWindow* dataDialog;
    GLMotif::Label*       dataDemFile;
    GLMotif::Label*       dataColorFile;

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

    PaletteEditor* paletteEditor;

    SpecularSettingsDialog specularSettings;

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
