#ifndef _Crusta_SurfaceProjector_H_
#define _Crusta_SurfaceProjector_H_


#include <GL/GLContextData.h>
#include <Vrui/InputDevice.h>

#include <crusta/CrustaComponent.h>
#include <crusta/SurfacePoint.h>


BEGIN_CRUSTA


class SurfaceProjector : public CrustaComponent
{
public:
    SurfaceProjector();

    SurfacePoint project(Vrui::InputDevice* device);

    void display(GLContextData& contextData,
                 const Vrui::NavTransform& original,
                 const Vrui::NavTransform& transform) const;

protected:
    bool projectionFailed;
};


END_CRUSTA


#endif //_Crusta_SurfaceProjector_H_
