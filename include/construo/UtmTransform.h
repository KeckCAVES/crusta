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

#ifndef _UtmTransfrom_H_
#define _UtmTransfrom_H_

#include <Math/Math.h>

#include <construo/ImageTransform.h>

BEGIN_CRUSTA

class UtmTransform : public ImageTransform
{
public:
    ///enumerated types for UTM hemispheres
	enum Hemisphere
    {
		NORTH,
        SOUTH
    };
	
    ///dummy constructor
	UtmTransform();
    ///constructor with given UTM zone and hemisphere
	UtmTransform(int sZone, Hemisphere sHemisphere);

    ///sets a new geoid major radius and flattening factor
	void setGeoid(double newA, double newF);
    ///sets the UTM zone
	void setZone(int newZone);
    ///sets the UTM hemisphere
	void setHemisphere(Hemisphere newHemisphere);
    ///returns the UTM zone
	int getZone() const;
    ///returns the UTM hemisphere
	Hemisphere getHemisphere() const;

	virtual Point systemToWorld(const Point& systemPoint) const;
	virtual Point worldToSystem(const Point& worldPoint) const;
	virtual Box systemToWorld(const Box& systemBox) const;
	virtual Box worldToSystem(const Box& worldBox) const;
	virtual Point imageToWorld(const Point& imagePoint) const;
	virtual Point imageToWorld(int imageX,int imageY) const;
	virtual Point worldToImage(const Point& worldPoint) const;
	virtual bool isCompatible(const ImageTransform& other) const;
	virtual size_t getFileSize() const;

private:
	/* Geoid parameters: */
	double a;                        ///< Major radius of geoid
	double f;                        ///< Geoid flattening factor
	double e2;                       ///< Derived flattening factor
	double Mc1,Mc2,Mc3,Mc4;          ///< Coefficients of the geoid formula
	double e1;                       ///< Derived inverse flattening factor
	double IMc0,IMc1,IMc2,IMc3,IMc4; ///< Coefficients of the inverse geoid
	
	/* UTM projection parameters: */
	double k0;  ///< Stretching at center parallel
	double ep2; ///< Derived projection factor
	
	/* UTM parameters: */
	int zone;              ///< UTM zone
	double lng0;           ///< Longitude of zone's central meridian
	Hemisphere hemisphere; ///< UTM hemisphere
    ///false northing; 0 for northern hemisphere, 10e6 for southern hemisphere
	double M0; 
    
    ///computes the UTM projection constants based on the current geoid setting
	void calcProjectionConstants();
};

END_CRUSTA

#endif //_UtmTransfrom_H_
