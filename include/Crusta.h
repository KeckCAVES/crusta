#ifndef _Crusta_H_
#define _Crusta_H_

#include <Vrui/Vislet.h>

///\todo remove once other stuff has been added?
#include <basics.h>
#if 0
#include <QuadTree.h>
#include <ViewLod.h>
#include <FrustumVisibility.h>
#include <dummies.h>
#endif

namespace Vrui {
    class VisletManager;
}

BEGIN_CRUSTA

class GlobalGrid;

///\todo comment properly
class CrustaFactory : public Vrui::VisletFactory
{
    friend class Crusta;

public:
    CrustaFactory(Vrui::VisletManager& visletManager);
    virtual ~CrustaFactory();

//- inherited from VisletFactory
public:
    virtual Vrui::Vislet* createVislet(
        int numVisletArguments, const char* const visletArguments[]) const;
    virtual void destroyVislet(Vrui::Vislet* vislet) const;
};

/**
    The main controlling component of the 'Crusta' application
 
    ///\todo what does it do, etc.
*/
class Crusta : public Vrui::Vislet
{
    friend class CrustaFactory;

public:
    Crusta();
    ~Crusta();

private:
    /** handle to the factory object for this class */
    static CrustaFactory* factory;

#if 0
///\todo remove. Debug shit
QuadTree* tree;
ViewLod* lod;
FrustumVisibility* visibility;
#endif
    GlobalGrid* globalGrid;

//- inherited from Vislet
public:
    virtual Vrui::VisletFactory* getFactory() const;

    virtual void frame();
    virtual void display(GLContextData& contextData) const;
};


END_CRUSTA

#endif //_Crusta_H_
