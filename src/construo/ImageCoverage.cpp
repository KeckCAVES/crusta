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

#include <construo/ImageCoverage.h>

#include <cstring>
#include <cstdio>
#include <Misc/File.h>
#include <Misc/ThrowStdErr.h>

namespace crusta {

ImageCoverage::
ImageCoverage(void) :
    numVertices(0), vertices(0), bbox(Box::empty)
{}

ImageCoverage::
ImageCoverage(int sNumVertices) :
	numVertices(sNumVertices), vertices(new Point[numVertices]),
    bbox(Box::empty)
{}

ImageCoverage::
ImageCoverage(const char* fileName) :
	vertices(0), bbox(Box::empty)
{
	/* Open the file: */
	Misc::File file(fileName,"rt");
	char line[256];
	
	/* Read the number of vertices: */
	file.gets(line,sizeof(line));
	if(sscanf(line,"%d",&numVertices)!=1)
    {
		Misc::throwStdErr("ImageCoverage::ImageCoverage: Invalid coverage file "
                          "%s", fileName);
    }
	
	/* Allocate the vertex array: */
	vertices=new Point[numVertices];
	
	/* Read the vertices and calculate the bounding box: */
	for(int vertexIndex=0;vertexIndex<numVertices;++vertexIndex)
    {
		file.gets(line,sizeof(line));
		double p[2];
		if(sscanf(line,"%lf %lf",&p[0],&p[1])!=2)
        {
			Misc::throwStdErr("ImageCoverage::ImageCoverage: Invalid coverage "
                              "file %s",fileName);
        }

		vertices[vertexIndex]=Point(p);
		bbox.addPoint(vertices[vertexIndex]);
    }
}

ImageCoverage::
~ImageCoverage(void)
{
	delete[] vertices;
}

void ImageCoverage::
setNumVertices(int newNumVertices)
{
	if(numVertices!=newNumVertices)
    {
		delete[] vertices;
		numVertices=newNumVertices;
		vertices=new Point[numVertices];
    }
}

void ImageCoverage::
setVertex(int vertexIndex,const Point& newVertex)
{
	vertices[vertexIndex]=newVertex;
}

void ImageCoverage::
finalizeVertices(void)
{
	/* Recalculate the bounding box: */
	bbox=Box::empty;
	for(int vertexIndex=0;vertexIndex<numVertices;++vertexIndex)
		bbox.addPoint(vertices[vertexIndex]);
}

void ImageCoverage::
flip(int imageHeight)
{
	/* Invert the y coordinates of all control points and recalculate the bounding box: */
	bbox=Box::empty;
	for(int vertexIndex=0;vertexIndex<numVertices;++vertexIndex)
    {
		vertices[vertexIndex][1] = Point::Scalar(imageHeight-1) -
                                   vertices[vertexIndex][1];
		bbox.addPoint(vertices[vertexIndex]);
    }
}

int ImageCoverage::
getNumVertices() const
{
    return numVertices;
}

const Point& ImageCoverage::
getVertex(int vertexIndex) const
{
    return vertices[vertexIndex];
}

const Box& ImageCoverage::
getBoundingBox() const
{
    return bbox;
}

bool ImageCoverage::
contains(const Point& p) const
{
	/* Check the point for region changes against all polygon edges: */
	int numRegionChanges=0;
	const Point* prev=&vertices[numVertices-1];
	for(int i=0;i<numVertices;++i)
    {
		const Point* curr=&vertices[i];
		if((*prev)[1]<=p[1]&&(*curr)[1]>p[1])
        {
			if((p[0]-(*prev)[0])*((*curr)[1]-(*prev)[1]) <
               (p[1]-(*prev)[1])*((*curr)[0]-(*prev)[0]))
            {
				++numRegionChanges;
            }
        }
		else if((*prev)[1]>p[1]&&(*curr)[1]<=p[1])
        {
			if((p[0]-(*curr)[0])*((*prev)[1]-(*curr)[1]) <
               (p[1]-(*curr)[1])*((*prev)[0]-(*curr)[0]))
            {
				++numRegionChanges;
            }
        }
		prev=curr;
    }
	
	return (numRegionChanges&0x1)!=0x0;
}

int ImageCoverage::
overlaps(const Box& box) const
{
	/* Check box against the bounding box first: */
	if(!bbox.overlaps(box))
		return 0;
	
	/* Check if any polygon edges intersect the box, and check the box's min
       vertex for region changes against all polygon edges: */
	int numRegionChanges=0;
	const Point* prev=&vertices[numVertices-1];
	for(int vertexIndex=0;vertexIndex<numVertices;++vertexIndex)
    {
		const Point* curr=&vertices[vertexIndex];
		
		//check if the current edge intersects the box or is contained by it:
        Point::Scalar lambda0=Point::Scalar(0);
		Point::Scalar lambda1=Point::Scalar(1);
		for(int i=0;i<2;++i)
        {
			/* Check against min box edge: */
			if((*prev)[i]<box.min[i])
            {
				if((*curr)[i]>=box.min[i])
                {
					Point::Scalar lambda = (box.min[i]-(*prev)[i]) /
                                            ((*curr)[i]-(*prev)[i]);
					if(lambda0<lambda)
						lambda0=lambda;
                }
				else
					lambda0=Point::Scalar(1);
            }
			else if((*curr)[i]<box.min[i])
            {
				if((*prev)[i]>=box.min[i])
                {
					Point::Scalar lambda=(box.min[i]-(*prev)[i]) /
                                  ((*curr)[i]-(*prev)[i]);
					if(lambda1>lambda)
						lambda1=lambda;
                }
				else
					lambda1=Point::Scalar(0);
            }
			
			/* Check against max box edge: */
			if((*prev)[i]>box.max[i])
            {
				if((*curr)[i]<=box.max[i])
                {
					Point::Scalar lambda=(box.max[i]-(*prev)[i]) /
                                  ((*curr)[i]-(*prev)[i]);
					if(lambda0<lambda)
						lambda0=lambda;
                }
				else
					lambda0=Point::Scalar(1);
            }
			else if((*curr)[i]>box.max[i])
            {
				if((*prev)[i]<=box.max[i])
                {
					Point::Scalar lambda=(box.max[i]-(*prev)[i]) /
                                  ((*curr)[i]-(*prev)[i]);
					if(lambda1>lambda)
						lambda1=lambda;
                }
				else
					lambda1=Point::Scalar(0);
            }
        }
		
		//box overlaps if current edge is contained by or intersects the box:
		if(lambda0<lambda1)
			return 1;
		
		/* Check the box's min vertex for region changes against the current
           edge: */
		if((*prev)[1]<=box.min[1]&&(*curr)[1]>box.min[1])
        {
			if((box.min[0]-(*prev)[0])*((*curr)[1]-(*prev)[1]) <
               (box.min[1]-(*prev)[1])*((*curr)[0]-(*prev)[0]))
            {
				++numRegionChanges;
            }
        }
		else if((*prev)[1]>box.min[1]&&(*curr)[1]<=box.min[1])
        {
			if((box.min[0]-(*curr)[0])*((*prev)[1]-(*curr)[1]) <
               (box.min[1]-(*curr)[1])*((*prev)[0]-(*curr)[0]))
            {
				++numRegionChanges;
            }
        }
		prev=curr;
    }
	
	/* Box overlaps if its min vertex is contained in the polygon: */
	return (numRegionChanges&0x1)!=0x0?2:0;
}

} //namespace crusta
