#include <crusta/VruiCoordinateTransform.h>
#include <crusta/Crusta.h>

BEGIN_CRUSTA

VruiCoordinateTransform::VruiCoordinateTransform():
  CrustaComponent(NULL), geoid(SETTINGS->globeRadius, 0.0)
{}

const char* VruiCoordinateTransform::getComponentName(int componentIndex) const
{
  switch(componentIndex)
  {
    case 0: return "Longitude";
    case 1: return "Latitude";
    case 2: return "Elevation";
    default: return "";
  }
}

const char* VruiCoordinateTransform::getUnitName(int componentIndex) const
{
  switch(componentIndex)
  {
    case 0:
    case 1: return "degree";
    case 2: return "meter";
    default: return "";
  }
}

const char* VruiCoordinateTransform::getUnitAbbreviation(int componentIndex) const
{
  switch(componentIndex)
  {
    case 0:
    case 1: return "deg";
    case 2: return "m";
    default: return "";
  }
}

Vrui::Point VruiCoordinateTransform::transform(const Vrui::Point& navigationPoint) const
{
  Vrui::Point lle=geoid.cartesianToGeodetic(crusta->mapToUnscaledGlobe(navigationPoint));
  lle[0]=Math::deg(lle[0]); lle[1]=Math::deg(lle[1]);
  return lle;
}

Vrui::Point VruiCoordinateTransform::inverseTransform(const Vrui::Point& userPoint) const
{
  Vrui::Point pos=userPoint;
  pos[0]=Math::rad(pos[0]); pos[1]=Math::rad(pos[1]);
  return crusta->mapToScaledGlobe(geoid.geodeticToCartesian(pos));
}

END_CRUSTA
