#ifndef _CrustaApp_H_
#define _CrustaApp_H_

#include <GLMotif/ToggleButton.h>
#include <GLMotif/Slider.h>

#include <Vrui/Application.h>
#include <Vrui/Geometry.h>

#include <crusta/basics.h>

class GLContextData;

namespace GLMotif {
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
	void produceMainMenu();
	void produceToolSubMenu(GLMotif::Menu* mainMenu);
    void produceVerticalScaleDialog();
    void produceLightingDialog();

    void alignSurfaceFrame(Vrui::NavTransform& surfaceFrame);

    void showVerticalScaleCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void debugSpheresCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void debugGridCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void changeScaleCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
	void resetNavigationCallback(Misc::CallbackData* cbData);

	void showLightingDialogCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void updateSun(void);
	void enableSunToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void sunAzimuthSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
	void sunElevationSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);

    double newVerticalScale;

    GLMotif::PopupMenu*   popMenu;
    GLMotif::RadioBox*    curTool;
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

    /** the crusta instance */
    Crusta* crusta;

//- inherited from Vrui::Application
public:
    virtual void frame();
    virtual void display(GLContextData& contextData) const;
	virtual void toolCreationCallback(
        Vrui::ToolManager::ToolCreationCallbackData* cbData);
};

END_CRUSTA

#endif //_CrustaApp_H_
