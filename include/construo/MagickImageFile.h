/***********************************************************************
MagickImageFile - Class to represent image files read via ImageMagick.
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

#ifndef _MagickImageFile_H_
#define _MagickImageFile_H_

#include <Magick++.h>

#include <construo/ColorTextureSpecs.h>
#include <construo/ImageFile.h>

class MagickImageFile : public ImageFile<TextureColor>
{
public:
    ///type of base class
	typedef ImageFile<TextureColor> Base;

    ///opens an image file by name
	MagickImageFile(const char* imageFileName);
    ///closes the image file
	virtual ~MagickImageFile();

private:
    ///imageMagick image object holding the pixel data
	Magick::Image image;

//- inherited from ImageFileBase
public:
	virtual void readRectangle(const uint rectOrigin[2], const uint rectSize[2],
                               Pixel* rectBuffer) const;
};

#endif //_MagickImageFile_H_
