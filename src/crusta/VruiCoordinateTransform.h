#ifndef _VruiCoordinateTransform_H_
#define _VruiCoordinateTransform_H_

#include <Geometry/Geoid.h>
#include <Vrui/CoordinateTransform.h>
#include <crusta/CrustaComponent.h>

namespace crusta {

class VruiCoordinateTransform: public Vrui::CoordinateTransform, public CrustaComponent
{
  public:
  VruiCoordinateTransform();
  virtual const char* getComponentName(int componentIndex) const;
  virtual const char* getUnitName(int componentIndex) const;
  virtual const char* getUnitAbbreviation(int componentIndex) const;
  virtual Vrui::Point transform(const Vrui::Point& navigationPoint) const;
  virtual Vrui::Point inverseTransform(const Vrui::Point& userPoint) const;
  Geometry::Geoid<double> geoid;
};

} //namespace crusta

#endif
