#ifndef _CrustaApp_H_
#define _CrustaApp_H_

#include <GLMotif/PopupMenu.h>
#include <Vrui/Application.h>

#include <crusta/basics.h>

class GLContextData;

BEGIN_CRUSTA

class Crusta;

class CrustaApp : public Vrui::Application
{
public:
    CrustaApp(int& argc, char**& argv, char**& appDefaults);
    ~CrustaApp();

private:
	void produceMainMenu();
	void resetNavigationCallback(Misc::CallbackData* cbData);

    /** handle to the core crusta instance */
    Crusta* crusta;

    GLMotif::PopupMenu*   popMenu;

//- inherited from Vrui::Application
public:
    virtual void frame();
    virtual void display(GLContextData& contextData) const;
};

END_CRUSTA

#endif //_CrustaApp_H_
