#include <Terrain.h>

#include <GL/GLGeometryWrappers.h>

#include <GlobalGrid.h>

BEGIN_CRUSTA

void Terrain::
displayCallback(void* object, const gridProcessing::ScopeData& scopeData)
{
/**\todo shite this is horrible. Need to optimize by preparing the traversal
    once for each phase, having an Pre-traversal callback, traversal and
    post-traversal one.
*/
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glLineWidth(1.0f);
	glColor3f(1.0f,1.0f,1.0f);
    
    glBegin(GL_QUADS);
    for(int i=0; i<4; ++i)
    {
        glNormal(scopeData.scope.corners[i] - Point::origin);
        glVertex(scopeData.scope.corners[i]);
    }
    glEnd();

    glPopAttrib();
}

void Terrain::
registerToGrid(GlobalGrid* grid)
{
    gridProcessing::ScopeCallback displaying(this, &Terrain::displayCallback);
    grid->registerScopeCallback(gridProcessing::DISPLAY_PHASE, displaying);
}

END_CRUSTA
