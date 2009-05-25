/***********************************************************************
GeoTransform - Class for transformations from system to world
coordinates where system coordinates are already in geographic
coordinates (lat/long).
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

#include <construo/Converters.h>
#include <construo/GeoTransform.h>

BEGIN_CRUSTA

Point::Scalar GeoTransform::
getFinestResolution(const int size[2]) const
{
/**\todo compute this properly, for now approximate by just taking a pixel
         closest to the pole */
    Point br = imageToWorld(0, 0);
    Point tl = imageToWorld(size[0]-1, size[1]-1);
    Point start;
    start[0] = (br[0]+tl[0])*0.5;
    start[1] = (Math::Constants<Point::Scalar>::pi-Math::abs(br[1])) <
        (Math::Constants<Point::Scalar>::pi-Math::abs(tl[1])) ? br[1] : tl[1];

    Point end(start[0]+Math::rad(scale[0]), start[1]);
    Point::Scalar lx = Converter::haversineDist(start, end, SPHEROID_RADIUS);
    end[0] = start[0];
    end[1] = start[1] + Math::rad(scale[1]);
    Point::Scalar ly = Converter::haversineDist(start, end, SPHEROID_RADIUS);
    
    return std::min(lx, ly);
}

Point GeoTransform::
systemToWorld(const Point& systemPoint) const
{
    return Point(Math::rad(systemPoint[0]), Math::rad(systemPoint[1]));
}

Point GeoTransform::
worldToSystem(const Point& worldPoint) const
{
    return Point(Math::deg(worldPoint[0]), Math::deg(worldPoint[1]));
}

Box GeoTransform::
systemToWorld(const Box& systemBox) const
{
    Point min,max;
    for(int i=0;i<2;++i)
    {
        min[i]=Math::rad(systemBox.min[i]);
        max[i]=Math::rad(systemBox.max[i]);
    }
    return Box(min,max);
}

Box GeoTransform::
worldToSystem(const Box& worldBox) const
{
    Point min,max;
    for(int i=0;i<2;++i)
    {
        min[i]=Math::deg(worldBox.min[i]);
        max[i]=Math::deg(worldBox.max[i]);
    }
    return Box(min,max);
}

Point GeoTransform::
imageToWorld(const Point& imagePoint) const
{
    return Point(Math::rad(imagePoint[0]*scale[0] + offset[0]),
                Math::rad(imagePoint[1]*scale[1] + offset[1]));
}

Point GeoTransform::
imageToWorld(int imageX,int imageY) const
{
    return Point(Math::rad(Point::Scalar(imageX)*scale[0] + offset[0]),
                 Math::rad(Point::Scalar(imageY)*scale[1] + offset[1]));
}

Point GeoTransform::
worldToImage(const Point& worldPoint) const
{
    return Point((Math::deg(worldPoint[0]) - offset[0]) / scale[0],
                 (Math::deg(worldPoint[1]) - offset[1]) / scale[1]);
}

Box GeoTransform::
imageToWorld(const Box& imageBox) const
{
    Point min,max;
    for(int i=0;i<2;++i)
    {
        if(scale[i]>=Point::Scalar(0))
        {
            min[i]=Math::rad(imageBox.min[i]*scale[i] + offset[i]);
            max[i]=Math::rad(imageBox.max[i]*scale[i] + offset[i]);
        }
        else
        {
            min[i]=Math::rad(imageBox.max[i]*scale[i] + offset[i]);
            max[i]=Math::rad(imageBox.min[i]*scale[i] + offset[i]);
        }
    }
    return Box(min,max);
}

Box GeoTransform::
worldToImage(const Box& worldBox) const
{
    Point min,max;
    for(int i=0;i<2;++i)
    {
        if(scale[i]>=Point::Scalar(0))
        {
            min[i]=(Math::deg(worldBox.min[i]) - offset[i]) / scale[i];
            max[i]=(Math::deg(worldBox.max[i]) - offset[i]) / scale[i];
        }
        else
        {
            min[i]=(Math::deg(worldBox.max[i]) - offset[i]) / scale[i];
            max[i]=(Math::deg(worldBox.min[i]) - offset[i]) / scale[i];
        }
    }
    return Box(min,max);
}

bool GeoTransform::
isCompatible(const ImageTransform& other) const
{
	if(dynamic_cast<const GeoTransform*>(&other)==0)
		return false;
    
	return isSystemCompatible(other);
}

END_CRUSTA
