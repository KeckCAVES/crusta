#ifndef _CrustaApp_H_
#define _CrustaApp_H_

#include <GLMotif/ToggleButton.h>
#include <GLMotif/Slider.h>

#include <Vrui/Application.h>

#include <crusta/basics.h>

class GLContextData;

namespace GLMotif {
    class Label;
    class PopupMenu;
    class PopupWindow;
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
    void produceVerticalScaleDialog();

    void showVerticalScaleCallback(
        GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
    void changeScaleCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
	void resetNavigationCallback(Misc::CallbackData* cbData);

    /** handle to the core crusta instance */
    Crusta* crusta;

    double newVerticalScale;

    GLMotif::PopupMenu*   popMenu;
    GLMotif::PopupWindow* verticalScaleDialog;
	GLMotif::Label*       verticalScaleLabel;

//- inherited from Vrui::Application
public:
    virtual void frame();
    virtual void display(GLContextData& contextData) const;
};

END_CRUSTA

#endif //_CrustaApp_H_
