#ifndef _Shape_H_
#define _Shape_H_


#include <crusta/basics.h>


BEGIN_CRUSTA


///\todo comment on this more once the interface is stable
/**
    Abstract interface to shapes used for mapping.
*/
class Shape
{
public:
    enum ControlType
    {
        CONTROL_POINT,
        CONTROL_SEGMENT
    };
    enum End
    {
        END_FRONT,
        END_BACK
    };

    union Id
    {
        Id();
        Id(int iRaw);
        Id(int iType, int iIndex);
        bool operator ==(const Id& other) const;
        bool operator !=(const Id& other) const;

        struct
        {
            int type   :  8;
            int index  : 24;
        };
        int raw;
    };

    virtual ~Shape();

    Id select         (const Point3& pos, double& dist);
    Id selectPoint    (const Point3& pos, double& dist);
    Id selectSegment  (const Point3& pos, double& dist);
    Id selectExtremity(const Point3& pos, double& dist, End& end);

    bool isValid       (const Id& id);
    bool isValidPoint  (const Id& id);
    bool isValidSegment(const Id& id);

    virtual Id addControlPoint(const Point3& pos, End end=END_BACK);
    bool moveControlPoint(const Id& id, const Point3& pos);
    virtual void removeControlPoint(const Id& id);

    Id previousControl(const Id& id);
    Id nextControl(const Id& id);

    Point3&        getControlPoint(const Id& id);
    Point3s&       getControlPoints();
    const Point3s& getControlPoints() const;

    Id refine(const Id& id, const Point3& pos);

    static const Id BAD_ID;

protected:
    Point3s controlPoints;
};

END_CRUSTA


#endif //_Shape_H_
