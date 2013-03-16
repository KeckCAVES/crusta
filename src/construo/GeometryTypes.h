///\todo fix GPL

/***********************************************************************
ImageTypes - Declaration of common data types for image processing.
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

#ifndef _GeometryTypes_H_
#define _GeometryTypes_H_

#include <crustacore/basics.h>

#include <construo/vrui.h>

namespace crusta {

typedef Geometry::Point<double,      2> Point;
typedef Geometry::Box<Point::Scalar, 2> Box;

} //namespace crusta

#endif //_GeometryTypes_H_
