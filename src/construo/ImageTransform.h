/***********************************************************************
ImageTransform - Base class for transformations from image pixel
coordinates [0, size) to geographical coordinates.
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

#ifndef _ImageTransform_H_
#define _ImageTransform_H_

#include <Math/Math.h>

#include <construo/GeometryTypes.h>

BEGIN_CRUSTA

class ImageTransform
{
public:
    ImageTransform();
    virtual ~ImageTransform() {}

    ///set the grid sampling convention
    void setPointSampled(bool isPointSampled, const int* imageSize=NULL);
    ///retrieve the grid sampling convention
    bool getPointSampled() const;

    ///sets the image transform scale factors and offset values
    void setImageTransformation(const Point::Scalar newScale[2],
                                const Point::Scalar newOffset[2]);
    /** flips the image transformation vertically according to the given image
        height */
    virtual void flip(int imageHeight);
    ///returns the size in meters of the shortest side of the smallest pixel
    virtual Point::Scalar getFinestResolution(const int size[2]) const = 0;
    ///returns scaling factors as array
    const Point::Scalar* getScale() const;
    ///returns offset values as array
    const Point::Scalar* getOffset() const;

    ///converts a point in image coordinates to system coordinates
    virtual Point imageToSystem(const Point& imagePoint) const;
    ///ditto, with integer image coordinate
    virtual Point imageToSystem(int imageX, int imageY) const;
    ///converts a point in system coordinates to image coordinates
    virtual Point systemToImage(const Point& systemPoint) const;
    ///converts a box in image coordinates to system coordinates
    virtual Box imageToSystem(const Box& imageBox) const;
    ///converts a box in system coordinates to image coordinates
    virtual Box systemToImage(const Box& systemBox) const;

    ///converts a point from system coordinates to world coordinates
    virtual Point systemToWorld(const Point& systemPoint) const =0;
    ///converts a point from world coordinates to system coordinates
    virtual Point worldToSystem(const Point& worldPoint) const =0;
    ///converts a box from system coordinates to world coordinates
    virtual Box systemToWorld(const Box& systemBox) const =0;
    ///converts a box from world coordinates to system coordinates
    virtual Box worldToSystem(const Box& worldBox) const =0;

    ///directly converts a point from image to world coordinates
    virtual Point imageToWorld(const Point& imagePoint) const;
    ///ditto, with integer image coordinate
    virtual Point imageToWorld(int imageX, int imageY) const;
    ///directly converts a point from world to image coordinates
    virtual Point worldToImage(const Point& worldPoint) const;
    ///directly converts a box from image to world coordinates
    virtual Box imageToWorld(const Box& imageBox) const;
    ///directly converts a box from world to image coordinates
    virtual Box worldToImage(const Box& worldBox) const;

    ///checks if two image transformations are pixel-to-pixel compatible
    virtual bool isCompatible(const ImageTransform& other) const = 0;
    ///returns the size of the transformation when written to a binary file
    virtual size_t getFileSize() const;

protected:
    ///returns true if two image transformations are system-wise compatible
    bool isSystemCompatible(const ImageTransform& other) const;

    /** flag relating the sampling convention used for the grid. If false a
        texel represents the value of the area covered by the texel. If true
        it represents the value at the center of the cell */
    bool pointSampled;
    /** size of the image (needed for area sampled grids) */
    int imageSize[2];

    ///scale factors from pixel coordinates to system coordinates
    Point::Scalar scale[2];
    ///offset factors from pixel coordinates to system coordinates
    Point::Scalar offset[2];
};

END_CRUSTA

#endif //_ImageTransform_H_
