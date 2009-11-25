#ifndef _PolylineTool_H_
#define _PolylineTool_H_

#include <crusta/map/MapTool.h>


BEGIN_CRUSTA


class PolylineTool : public MapTool<Polyline>
{
    friend class Vrui::GenericToolFactory<PolylineTool>;

public:
    typedef Vrui::GenericToolFactory<PolylineTool> Factory;

    static void init();

protected:
    Factory* factory;

//- Inherited from Vrui::Tool
public:
    virtual const Vrui::ToolFactory* getFactory() const;
};


END_CRUSTA


#endif //_PolylineTool_H_
