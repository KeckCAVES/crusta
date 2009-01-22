#ifndef _SpheroidGrid_H_
#define _SpheroidGrid_H_

#include <GlobalGrid.h>

#include <GL/GLObject.h>

#include <gridProcessing.h>

BEGIN_CRUSTA

class QuadTree;
class FrustumVisibility;
class ViewLod;

class SpheroidGrid : public GlobalGrid, public GLObject
{
public:
    SpheroidGrid();
    ~SpheroidGrid();

protected:
    struct ViewData : public GLObject::DataItem
    {
        ViewData();
        ~ViewData();

        std::vector<QuadTree*> basePatches;
        FrustumVisibility* visibility;
        ViewLod* lod;
    };
    
    gridProcessing::Id newClientId;

//- inherited from GlobalGrid
public:
    virtual void frame();
    virtual void display(GLContextData& contextData) const;
    virtual void registerClient(
        const gridProcessing::RegistrationCallback callback);

//- inherited from GLObject
public:
    void initContext(GLContextData& contextData) const;
};

END_CRUSTA

#endif //_SpheroidGrid_H_
