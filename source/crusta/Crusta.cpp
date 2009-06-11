#include <crusta/Crusta.h>

#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>

#include <crusta/DataManager.h>
#include <crusta/QuadCache.h>
#include <crusta/QuadTerrain.h>
#include <crusta/Triacontahedron.h>

BEGIN_CRUSTA

/* start the frame counting at 2 because initialization code uses unsigneds that
   are initialized with 0. Thus if crustaFrameNumber starts at 0, the init code
   wouldn't be able to retrieve any cache buffers since all the buffers of the
   current and previous frame are locked */
FrameNumber Crusta::currentFrame   = 2;
FrameNumber Crusta::lastScaleFrame = 2;

double Crusta::verticalScale = 1.0;

DataManager* Crusta::dataMan = NULL;

Crusta::RenderPatches Crusta::renderPatches;

Crusta::Actives Crusta::actives;
Threads::Mutex Crusta::activesMutex;

void Crusta::
init(const std::string& demFileBase, const std::string& colorFileBase)
{
    Triacontahedron polyhedron(SPHEROID_RADIUS);

    dataMan  = new DataManager(&polyhedron, demFileBase, colorFileBase);

    uint numPatches = polyhedron.getNumPatches();
    renderPatches.resize(numPatches);
    for (uint i=0; i<numPatches; ++i)
        renderPatches[i] = new QuadTerrain(i, polyhedron.getScope(i));
}

void Crusta::
shutdown()
{
    for (RenderPatches::iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        delete *it;
    }
}

const FrameNumber& Crusta::
getCurrentFrame()
{
    return currentFrame;
}

const FrameNumber& Crusta::
getLastScaleFrame()
{
    return lastScaleFrame;
}

void Crusta::
setVerticalScale(double newVerticalScale)
{
    verticalScale  = newVerticalScale;
    lastScaleFrame = currentFrame;
}

double Crusta::
getVerticalScale()
{
    return verticalScale;
}

DataManager* Crusta::
getDataManager()
{
    return dataMan;
}


void Crusta::
submitActives(const Actives& touched)
{
    Threads::Mutex::Lock lock(activesMutex);
    actives.insert(actives.end(), touched.begin(), touched.end());
}


void Crusta::
frame()
{
    ++currentFrame;

    DEBUG_OUT(8, "\n\n\n--------------------------------------\n%u\n\n\n",
              static_cast<unsigned int>(currentFrame));

    //make sure all the active nodes are current
    confirmActives();

    //process the requests from the last frame
    crustaQuadCache.getMainCache().frame();
}

void Crusta::
display(GLContextData& contextData)
{
    for (RenderPatches::const_iterator it=renderPatches.begin();
         it!=renderPatches.end(); ++it)
    {
        (*it)->display(contextData);
    }
}

void Crusta::
confirmActives()
{
    for (Actives::iterator it=actives.begin(); it!=actives.end(); ++it)
    {
        if (!(*it)->isCurrent())
            (*it)->getData().computeBoundingSphere();
        (*it)->touch();
    }
    actives.clear();
}


END_CRUSTA
