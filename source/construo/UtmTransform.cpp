/***********************************************************************
UtmTransform - Class for transformations from system to world
coordinates using Universal Transverse Mercator projection on the WGS-84
reference geoid.
Copyright (c) 2007-2008 Oliver Kreylos

This file is part of the DEM processing and visualization package.

The DEM processing and visualization package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The DEM processing and visualization package is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the DEM processing and visualization package; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

///\todo fix FRAK'ing cmake !@#!@
#define CONSTRUO_BUILD 1

#include <algorithm>
#include <construo/UtmTransform.h>

/*****************************
Methods of class UtmTransform:
*****************************/

BEGIN_CRUSTA

UtmTransform::
UtmTransform() :
    a(6378137.0), f(1.0/298.257), k0(0.9996)
{
	/* Calculate the projection constants: */
	calcProjectionConstants();
}

UtmTransform::
UtmTransform(int sZone, UtmTransform::Hemisphere sHemisphere) :
    a(6378137.0), f(1.0/298.257), k0(0.9996)
{
	/* Calculate the projection constants: */
	calcProjectionConstants();
	
	setZone(sZone);
	setHemisphere(sHemisphere);
}

void UtmTransform::
setGeoid(double newA, double newF)
{
	/* Set the geoid parameters: */
	a=newA;
	f=newF;
	
	/* Recalculate the projection constants: */
	calcProjectionConstants();
}

void UtmTransform::
setZone(int newZone)
{
	/* Set the UTM zone: */
	zone=newZone;
	
	/* Calculate longitude of zone's central meridian: */
	lng0=Math::rad(-177.0+double(zone-1)*6.0);
}

void UtmTransform::
setHemisphere(UtmTransform::Hemisphere newHemisphere)
{
	/* Set the UTM hemisphere: */
	hemisphere=newHemisphere;
	
	/* Set the false northing: */
	switch(hemisphere)
    {
		case NORTH:
			M0=0.0;
			break;
            
		case SOUTH:
			M0=10.0e6;
			break;
    }
}

int UtmTransform::
getZone() const
{
    return zone;
}

UtmTransform::Hemisphere UtmTransform::
getHemisphere() const
{
    return hemisphere;
}

Point::Scalar UtmTransform::
getFinestResolution(const int size[2]) const
{
///\todo consider distortion in UTM zone, for now just return the scale
    return std::min(Math::abs(scale[0]), Math::abs(scale[1]));
}

Point UtmTransform::
systemToWorld(const Point& systemPoint) const
{
    /* Calculate the reverse projection coefficients: */
    double M=(double(systemPoint[1])-M0)/k0;
    double mu=M/IMc0;
    double phi = mu + IMc1*Math::sin(2.0*mu) + IMc2*Math::sin(4.0*mu) +
                IMc3*Math::sin(6.0*mu) + IMc4*Math::sin(8.0*mu);
    double sphi=Math::sin(phi);
    double sphi2=Math::sqr(sphi);
    double cphi=Math::cos(phi);
    double cphi2=Math::sqr(cphi);
    double kappa=1.0-e2*sphi2;
    double N=a/Math::sqrt(kappa);
    double NbyR=kappa/(1.0-e2);
    double T=sphi2/cphi2;
    double C=ep2*cphi2;
    double D=(double(systemPoint[0])-500000.0)/(N*k0);
    
    /* Calculate the geographical coordinates: */
    double D2=Math::sqr(D);
    return Point(
        Point::Scalar(lng0 + ((((5.0 + (-3.0*C-2.0)*C + (24.0*T+28.0)*T +
                                 8.0*ep2) / 
                         120.0*D2-(1.0+C+2.0*T)/6.0)*D2+1.0)*D) / cphi),
        Point::Scalar(phi - NbyR*sphi/cphi * (((61.0 + (-3.0*C+298.0)*C +
                                         (45.0*T+90.0)*T - 252.0*ep2) /
                                        720.0*D2 - (5.0+(-4.0*C+10.0)*C +
                                                    3.0*T-9.0*ep2)/24.0) *
                                       D2+1.0/2.0)*D2));
}

Point UtmTransform::
worldToSystem(const Point& worldPoint) const
{
    /* Calculate the projection coefficients: */
    double sphi=Math::sin(double(worldPoint[1]));
    double sphi2=Math::sqr(sphi);
    double cphi=Math::cos(double(worldPoint[1]));
    double cphi2=Math::sqr(cphi);
    double N=a/Math::sqrt((1.0-e2*sphi2));
    double T=sphi2/cphi2;
    double C=ep2*cphi2;
    double A=(double(worldPoint[0])-lng0)*cphi;
    double M=(Mc1*double(worldPoint[1]) -
              Mc2*Math::sin(2.0*double(worldPoint[1])) +
              Mc3*Math::sin(4.0*double(worldPoint[1])) -
              Mc4*Math::sin(6.0*double(worldPoint[1]))) * a;
    
    /* Calculate the UTM coordinates: */
    double A2=Math::sqr(A);
    return Point(
        Point::Scalar(((1.0 +
                 ((1.0-T+C)+(5.0-18.0*T+T*T+72.0*C-58.0*ep2)*A2/20.0) *
                 A2/6.0)*A)*k0*N+500000.0),
        Point::Scalar((M +
                ((1.0+((5.0-T+9.0*C+4.0*C*C) +
                       (61.0-58.0*T+T*T+600.0*C-330.0*ep2) * 
                       A2/30.0) * 
                  A2/12.0) * A2/2.0)*N*sphi/cphi)*k0+M0));
}

Box UtmTransform::
systemToWorld(const Box& systemBox) const
{
	/* create the result box by transforming the system box's corners, and any
       central lat/long crossings: */
	Box result=Box::empty;
	for(int i=0;i<4;++i)
		result.addPoint(systemToWorld(systemBox.getVertex(i)));
	
	/* Check for central meridian crossings: */
	if(systemBox.min[0]<Point::Scalar(500000) &&
       systemBox.max[0]>Point::Scalar(500000))
    {
		if(systemBox.min[1]<M0)
        {
			result.addPoint(systemToWorld(Point(Point::Scalar(500000),
                                                systemBox.min[1])));
        }
		if(systemBox.max[1]>M0)
        {
			result.addPoint(systemToWorld(Point(Point::Scalar(500000),
                                                systemBox.max[1])));
        }
    }
	
	/* Check for equator crossings: */
	if(systemBox.min[1]<M0&&systemBox.max[1]>M0)
    {
		if(systemBox.min[0]>Point::Scalar(500000))
			result.addPoint(systemToWorld(Point(systemBox.min[0],M0)));
		else if(systemBox.max[0]<Point::Scalar(500000))
			result.addPoint(systemToWorld(Point(systemBox.max[0],M0)));
    }
	
	return result;
}

Box UtmTransform::
worldToSystem(const Box& worldBox) const
{
	/* Create the result box by transforming the system box's corners, and any
       central lat/long crossings: */
	Box result=Box::empty;
	for(int i=0;i<4;++i)
		result.addPoint(worldToSystem(worldBox.getVertex(i)));
	
	/* Check for central meridian crossings: */
	if(worldBox.min[0]<Point::Scalar(lng0) &&
       worldBox.max[0]>Point::Scalar(lng0))
    {
		if(worldBox.min[1]>Point::Scalar(0))
        {
			result.addPoint(worldToSystem(Point(
                Point::Scalar(lng0),worldBox.min[1])));
        }
		else if(worldBox.max[1]<Point::Scalar(0))
        {
			result.addPoint(worldToSystem(Point(
                Point::Scalar(lng0),worldBox.max[1])));
        }
    }
	
	/* Check for equator crossings: */
	if(worldBox.min[1]<Point::Scalar(0) && worldBox.max[1]>Point::Scalar(0))
    {
		if(worldBox.min[0]<Point::Scalar(lng0))
        {
			result.addPoint(worldToSystem(Point(worldBox.min[0],
                                                Point::Scalar(0))));
        }
		if(worldBox.max[0]>Point::Scalar(lng0))
        {
			result.addPoint(worldToSystem(Point(worldBox.max[0],
                                                Point::Scalar(0))));
        }
    }
	
	return result;
}

Point UtmTransform::
imageToWorld(const Point& imagePoint) const
{
    /* Calculate the reverse projection coefficients: */
    double M=(double(imagePoint[1]*scale[1]+offset[1])-M0)/k0;
    double mu=M/IMc0;
    double phi = mu + IMc1*Math::sin(2.0*mu) + IMc2*Math::sin(4.0*mu) +
                 IMc3*Math::sin(6.0*mu) + IMc4*Math::sin(8.0*mu);
    double sphi=Math::sin(phi);
    double sphi2=Math::sqr(sphi);
    double cphi=Math::cos(phi);
    double cphi2=Math::sqr(cphi);
    double kappa=1.0-e2*sphi2;
    double N=a/Math::sqrt(kappa);
    double NbyR=kappa/(1.0-e2);
    double T=sphi2/cphi2;
    double C=ep2*cphi2;
    double D=(double(imagePoint[0]*scale[0]+offset[0])-500000.0)/(N*k0);
    
    /* Calculate the geographical coordinates: */
    double D2=Math::sqr(D);
    return Point(
        Point::Scalar(lng0+((((5.0+(-3.0*C-2.0)*C+(24.0*T+28.0)*T+8.0*ep2) /
                       120.0*D2-(1.0+C+2.0*T)/6.0)*D2+1.0)*D)/cphi),
        Point::Scalar(phi-NbyR*sphi/cphi*(((61.0+(-3.0*C+298.0)*C +
                                    (45.0*T+90.0)*T -
                                     252.0*ep2)/720.0*D2 -
                                    (5.0+(-4.0*C+10.0)*C+3.0*T-9.0*ep2)/24.0) *
                                   D2+1.0/2.0)*D2));
}

Point UtmTransform::
imageToWorld(int imageX,int imageY) const
{
    Point imgPoint(imageX, imageY);
    return imageToWorld(imgPoint);
}

Point UtmTransform::
worldToImage(const Point& worldPoint) const
{
    /* Calculate the projection coefficients: */
    double sphi=Math::sin(double(worldPoint[1]));
    double sphi2=Math::sqr(sphi);
    double cphi=Math::cos(double(worldPoint[1]));
    double cphi2=Math::sqr(cphi);
    double N=a/Math::sqrt((1.0-e2*sphi2));
    double T=sphi2/cphi2;
    double C=ep2*cphi2;
    double A=(double(worldPoint[0])-lng0)*cphi;
    double M = (Mc1*double(worldPoint[1]) -
                Mc2*Math::sin(2.0*double(worldPoint[1])) +
                Mc3*Math::sin(4.0*double(worldPoint[1])) -
                Mc4*Math::sin(6.0*double(worldPoint[1])))*a;
    
    /* Calculate the UTM coordinates: */
    double A2=Math::sqr(A);
    return Point(
        (Point::Scalar(((1.0+((1.0-T+C)+
                              (5.0-18.0*T+T*T+72.0*C-58.0*ep2)*A2/20.0) *
                  A2/6.0)*A)*k0*N+500000.0)-offset[0])/scale[0],
        (Point::Scalar((M+((1.0+((5.0-T+9.0*C+4.0*C*C) +
                          (61.0-58.0*T+T*T+600.0*C-330.0*ep2) *
                          A2/30.0)*A2/12.0)*A2/2.0)*N*sphi/cphi)*k0+M0) -
         offset[1])/scale[1]);
}

bool UtmTransform::
isCompatible(const ImageTransform& other) const
{
	const UtmTransform* utmOther=dynamic_cast<const UtmTransform*>(&other);
	if(utmOther==0)
		return false;
	
	return zone==utmOther->zone&&hemisphere==utmOther->hemisphere&&isSystemCompatible(other);
}

size_t UtmTransform::
getFileSize() const
{
    ///writes base transformation and UTM zone
    return ImageTransform::getFileSize()+sizeof(int);
}


void UtmTransform::
calcProjectionConstants()
{
	e2=(2.0-f)*f;
	Mc1=1.0-(1.0+(3.0+5.0/4.0*e2)*e2/16.0)*e2/4.0;
	Mc2=(3.0+(3.0+45.0/32.0*e2)*e2/4.0)*e2/8.0;
	Mc3=(15.0+45.0/4.0*e2)*e2*e2/256.0;
	Mc4=35.0*e2*e2*e2/3072.0;
	e1=(1.0-Math::sqrt(1.0-e2))/(1.0+Math::sqrt(1.0-e2));
	IMc0=a*(((-5.0/256.0*e2-3.0/64.0)*e2-1.0/4.0)*e2+1.0);
	IMc1=(-27.0/32.0*e1*e1+3.0/2.0)*e1;
	IMc2=(-55.0/32.0*e1*e1+21.0/16.0)*e1*e1;
	IMc3=151.0/96.0*e1*e1*e1;
	IMc4=1097.0/512.0*e1*e1*e1*e1;
	ep2=e2/(1.0-e2);
}

END_CRUSTA
