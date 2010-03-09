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

    union ControlId
    {
        ControlId();
        ControlId(int iRaw);
        ControlId(int iType, int iIndex);
        bool operator ==(const ControlId& other) const;
        bool operator !=(const ControlId& other) const;

        struct
        {
            int type   :  8;
            int index  : 24;
        };
        int raw;
    };

    Shape();
    virtual ~Shape();

    ControlId select(const Point3& pos, double& dist, double pointBias=1.0);
    ControlId selectPoint(const Point3& pos, double& dist);
    ControlId selectSegment(const Point3& pos, double& dist);
    ControlId selectExtremity(const Point3& pos, double& dist, End& end);

    bool isValid(const ControlId& id);
    bool isValidPoint(const ControlId& id);
    bool isValidSegment(const ControlId& id);

    virtual ControlId addControlPoint(const Point3& pos, End end=END_BACK);
    virtual bool moveControlPoint(const ControlId& id, const Point3& pos);
    virtual void removeControlPoint(const ControlId& id);

    ControlId previousControl(const ControlId& id);
    ControlId nextControl(const ControlId& id);

    Symbol&        getSymbol();
    const Symbol&  getSymbol() const;
    Point3&        getControlPoint(const ControlId& id);
    Point3s&       getControlPoints();
    const Point3s& getControlPoints() const;

    ControlId refine(const ControlId& id, const Point3& pos);

    static const Symbol    DEFAULT_SYMBOL;
    static const ControlId BAD_ID;

protected:
    Symbol  symbol;
    Point3s controlPoints;
};

END_CRUSTA


#endif //_Shape_H_
