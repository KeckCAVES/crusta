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

    struct Symbol
    {
        int   id;
        Color color;

        Symbol();
        Symbol(int iId, const Color& iColor);
        Symbol(int iId, float iRed, float iGreen, float iBlue);
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

    Shape();
    virtual ~Shape();

    Id select         (const Point3& pos, double& dist, double pointBias=1.0);
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

    Symbol&        getSymbol();
    const Symbol&  getSymbol() const;
    Point3&        getControlPoint(const Id& id);
    Point3s&       getControlPoints();
    const Point3s& getControlPoints() const;

    Id refine(const Id& id, const Point3& pos);

    static const Symbol DEFAULT_SYMBOL;
    static const Id     BAD_ID;

protected:
    Symbol  symbol;
    Point3s controlPoints;
};

END_CRUSTA


#endif //_Shape_H_
