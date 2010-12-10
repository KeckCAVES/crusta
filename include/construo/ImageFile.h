/** \todo fix GPL */

/***********************************************************************
ImageFile - Base class to represent image files for simple reading of
individual tiles
Copyright (c) 2005-2008 Oliver Kreylos, Tony Bernardin

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

#ifndef _ImageFile_H_
#define _ImageFile_H_


#include <crusta/basics.h>


BEGIN_CRUSTA


template <typename PixelType>
class ImageFile
{
public:
    ImageFile();
    virtual ~ImageFile();

    /** set the uniform offset of the pixel values */
    void setPixelOffset(double offset);
    /** retrieve the uniform offset of the pixel values */
    double getPixelOffset() const;
    /** set the uniform scale of the pixel values */
    void setPixelScale(double scale);
    /** retrieve the uniform scale of the pixel values */
    double getPixelScale() const;
    /** retrieve the nodata value */
    const PixelType& getNodata() const;
    /** set the nodata value */
    void setNodata(const PixelType& nodataValue);
    /** returns image size */
    const int* getSize() const;
    /** reads a rectangle of pixel data into the given buffer */
    virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               PixelType* rectBuffer) const = 0;

protected:
    /** the uniform offset of the values of the image */
    double pixelOffset;
    /** the uniform scale of the values of the image */
    double pixelScale;
    /** the value corresponding to "no data" */
    PixelType nodata;
    /** size of the image in pixels (width x height) */
    int size[2];
};


END_CRUSTA


#include <construo/ImageFile.hpp>


#endif //_ImageFile_H_
