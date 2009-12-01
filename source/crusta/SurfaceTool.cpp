#include <crusta/SurfaceTool.h>

#include <Comm/MulticastPipe.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <crusta/Crusta.h>

BEGIN_CRUSTA


SurfaceTool::Factory* SurfaceTool::factory = NULL;

SurfaceTool::
SurfaceTool(const Vrui::ToolFactory* iFactory,
            const Vrui::ToolInputAssignment& inputAssignment) :
    Vrui::TransformTool(iFactory, inputAssignment)
{
}

SurfaceTool::
~SurfaceTool()
{
}


Vrui::ToolFactory* SurfaceTool::
init()
{
	Vrui::TransformToolFactory* transformToolFactory =
        dynamic_cast<Vrui::TransformToolFactory*>(
            Vrui::getToolManager()->loadClass("TransformTool"));

    Factory* surfaceFactory = new Factory("SurfaceTool", "Crusta Surface",
        transformToolFactory, *Vrui::getToolManager());

///\todo fix the number of buttons and valuators, for now hack in 2
	surfaceFactory->setNumDevices(1);
	surfaceFactory->setNumButtons(0, 2);

    Vrui::getToolManager()->addClass(surfaceFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    return surfaceFactory;
}


const Vrui::ToolFactory* SurfaceTool::
getFactory() const
{
    return factory;
}

void SurfaceTool::
frame()
{
    Vrui::InputDevice* device = input.getDevice(0);

    //transform the physical frame to navigation space
    Vrui::NavTransform physicalFrame = device->getTransformation();
    Vrui::NavTransform modelFrame    =
        Vrui::getInverseNavigationTransformation() * physicalFrame;

    //align the model frame to the surface
    Point3 surfacePoint;
    if (Vrui::isMaster())
    {
        surfacePoint =
            Crusta::snapToSurface(modelFrame.getOrigin());
    }
    else
        Vrui::getMainPipe()->read<Point3>(surfacePoint);

    modelFrame = Vrui::NavTransform(Vector3(surfacePoint),
        modelFrame.getRotation(), modelFrame.getScaling());

    //transform the aligned frame back to physical space
    physicalFrame = Vrui::getNavigationTransformation() * modelFrame;
    transformedDevice->setTransformation(Vrui::TrackerState(
        physicalFrame.getTranslation(), physicalFrame.getRotation()));
}

void SurfaceTool::
display(GLContextData& contextData) const
{
//    Vrui::NavTransform physicalFrame = transformedDevice->getTransformation();
    Vrui::NavTransform physicalFrame = input.getDevice(0)->getTransformation();
    Vrui::NavTransform modelFrame    =
        Vrui::getInverseNavigationTransformation() * physicalFrame;

    Point3 o = modelFrame.getOrigin();
    Point3 x = o + 10000.0*modelFrame.getDirection(0);
    Point3 y = o + 10000.0*modelFrame.getDirection(1);
    Point3 z = o + 10000.0*modelFrame.getDirection(2);

#if 0
    o = Point3(0,0,0);
    x = Point3(1,0,0);
    y = Point3(0,1,0);
    z = Point3(0,0,1);
#endif

    GLint activeTexture;
    glPushAttrib(GL_ENABLE_BIT);
    glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_LINES);
        glColor3f(1.0, 0.0, 0.0);
        glVertex3f(0,0,0);
        glVertex3f(o[0], o[1], o[2]);
//        glVertex3f(x[0], x[1], x[2]);
        glColor3f(0.0, 1.0, 0.0);
        glVertex3f(o[0], o[1], o[2]);
        glVertex3f(y[0], y[1], y[2]);
        glColor3f(0.0, 0.0, 1.0);
        glVertex3f(o[0], o[1], o[2]);
        glVertex3f(z[0], z[1], z[2]);
    glEnd();

    glPopAttrib();
    glActiveTexture(activeTexture);
}


END_CRUSTA
