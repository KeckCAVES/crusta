///\todo fix GPL

/***********************************************************************
ImageCoverage - Class defining the valid region of an image as a polygon
in pixel coordinates.
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

#ifndef _ImageCoverage_H_
#define _ImageCoverage_H_

#include <construo/GeometryTypes.h>

BEGIN_CRUSTA

///\todo conform to the style of SphereCoverage.
class ImageCoverage
{
public:
    ///dummy constructor
	ImageCoverage();
    ///creates uninitialized coverage polygon with given number of vertices
	ImageCoverage(int sNumVertices);
    ///reads coverage polygon from text file
	ImageCoverage(const char* fileName);
    ///destroys coverage polygon
	~ImageCoverage();

     /** deletes the current coverage polygon; creates uninitialized one with
         given number of vertices */
	void setNumVertices(int newNumVertices);
    ///sets one of the polygon's vertices
	void setVertex(int vertexIndex,const Point& newVertex);
    ///finalizes the coverage polygon after any changes
	void finalizeVertices();
    /** flips the coverage polygon vertices vertically according to the given
        image height */
	void flip(int imageHeight);
    ///returns number of vertices in current coverage polygon
	int getNumVertices() const;
    ///returns on vertex of the current coverage polygon
	const Point& getVertex(int vertexIndex) const;
    ///returns bounding box of current coverage polygon
	const Box& getBoundingBox() const;
    /** checks if the given point in image coordinates is inside the coverage
        region */
	bool contains(const Point& p) const;
    /** checks if the given axis-aligned box overlaps the coverage region:
        0 - separate, 1 - overlaps, 2 - contains */
	int overlaps(const Box& box) const;

private:
    ///number of vertices in polygon
	int numVertices;
    ///array of vertices in image coordinates
	Point* vertices;
    ///bounding box of the current coverage polygon in image coordinates
	Box bbox;
};

END_CRUSTA

#endif //_ImageCoverage_H_
