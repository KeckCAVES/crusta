/***********************************************************************
 GdalTransform - Class for transformations from system to world
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

#include <Geometry/AffineTransformation.h>
#include <Math/Math.h>

#include <GDAL/ogr_spatialref.h>
#include <GDAL/ogr_srs_api.h>

#include <construo/ImageTransform.h>

BEGIN_CRUSTA

class GdalTransform : public ImageTransform
{
public:
    ///dummy constructor
    GdalTransform();
    ///constructor with given well known text description of the transform
    GdalTransform(const char* projectionFileName);
    
	virtual void flip(int imageHeight);

    virtual Point::Scalar getFinestResolution(const int size[2]) const;

	virtual Point imageToSystem(const Point& imagePoint) const;
	virtual Point imageToSystem(int imageX, int imageY) const;
	virtual Point systemToImage(const Point& systemPoint) const;
	virtual Box imageToSystem(const Box& imageBox) const;
	virtual Box systemToImage(const Box& systemBox) const;

    virtual Point systemToWorld(const Point& systemPoint) const;
    virtual Point worldToSystem(const Point& worldPoint) const;
    virtual Box systemToWorld(const Box& systemBox) const;
    virtual Box worldToSystem(const Box& worldBox) const;

    virtual Point imageToWorld(const Point& imagePoint) const;
    virtual Point imageToWorld(int imageX,int imageY) const;
    virtual Point worldToImage(const Point& worldPoint) const;
	virtual Box imageToWorld(const Box& imageBox) const;
	virtual Box worldToImage(const Box& worldBox) const;
    
    virtual bool isCompatible(const ImageTransform& other) const;
    virtual size_t getFileSize() const;
    
private:
    typedef Geometry::AffineTransformation<Point::Scalar, 2> Transform;

    /** converts from pixel coordinates to georeferenced ones */
    Transform pixelToGeo;
    /** converts from georeferenced coordinates to pixel ones */
    Transform geoToPixel;
    /** converts from georeferenced coordinates to construo world ones */
    OGRCoordinateTransformation* geoToWorld;
    /** converts from construo world coordinates to georeferenced ones */
    OGRCoordinateTransformation* worldToGeo;
};

END_CRUSTA

#endif //_UtmTransfrom_H_
