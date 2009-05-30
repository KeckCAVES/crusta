#include <crusta/Crusta.h>

#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>

#include <crusta/QuadCache.h>
#include <crusta/Spheroid.h>

BEGIN_CRUSTA

/* start the frame counting at 2 because initialization code uses unsigneds that
   are initialized with 0. Thus if crustaFrameNumber starts at 0, the init code
   wouldn't be able to retrieve any cache buffers since all the buffers of the
   current and previous frame are locked */
uint crustaFrameNumber = 2;

double Crusta::verticalExaggeration = 1.0;

Crusta::
Crusta(const std::string& demFileBase, const std::string& colorFileBase) :
    spheroid(demFileBase, colorFileBase)
{}

double Crusta::
getVerticalExaggeration() const
{
    return verticalExaggeration;
}

void Crusta::
frame()
{
    ++crustaFrameNumber;

    DEBUG_OUT(8, "\n\n\n--------------------------------------\n%u\n\n\n",
              static_cast<unsigned int>(crustaFrameNumber));

    //process the requests from the last frame
    crustaQuadCache.getMainCache().frame();
}

void Crusta::
display(GLContextData& contextData) const
{
///\todo remove. Debug

//push everything to navigational coordinates for vislet
glDisable(GL_LIGHTING);
glEnable(GL_COLOR_MATERIAL);
glColor3f(1.0, 1.0, 1.0);
glPointSize(5.0f);
glBegin(GL_POINTS);
glVertex3f(0,0,0);
glEnd();

glColor3f(1.0, 0.0, 0.0);
glBegin(GL_LINES);
glVertex3f(0,0,0);
glVertex3f(1,0,0);
glEnd();

glColor3f(0.0, 1.0, 0.0);
glBegin(GL_LINES);
glVertex3f(0,0,0);
glVertex3f(0,1,0);
glEnd();

glColor3f(0.0, 0.0, 1.0);
glBegin(GL_LINES);
glVertex3f(0,0,0);
glVertex3f(0,0,1);
glEnd();

glEnable(GL_LIGHTING);
glDisable(GL_COLOR_MATERIAL);

    spheroid.display(contextData);
}

END_CRUSTA
