#include <crusta/CrustaApp.h>

#include <GL/GLContextData.h>

#include <crusta/Crusta.h>

BEGIN_CRUSTA

CrustaApp::
CrustaApp(int& argc, char**& argv, char**& appDefaults) :
    Vrui::Application(argc, argv, appDefaults), crusta(new Crusta)
{
}

CrustaApp::
~CrustaApp()
{
    delete crusta;
}


void CrustaApp::
frame()
{
    crusta->frame();
}

void CrustaApp::
display(GLContextData& contextData) const
{
    crusta->display(contextData);
}


END_CRUSTA


int main(int argc, char* argv[])
{
	char** appDefaults = 0; // This is an additional parameter no one ever uses
    crusta::CrustaApp app(argc, argv, appDefaults);
	
	app.run();
	
    return 0;
}