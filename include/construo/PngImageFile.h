///\todo fix GPL

/***********************************************************************
PngImageFile - Class to represent image files in PNG format.
Copyright (c) 2006-2008 Oliver Kreylos

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

#ifndef _PngImageFile_H_
#define _PngImageFile_H_

#include <construo/ImageFile.h>
#include <crusta/ColorTextureSpecs.h>

BEGIN_CRUSTA

class PngImageFile : public ImageFile<TextureColor>
{
public:
    ///type of base class
	typedef ImageFile<TextureColor> Base;

    ///opens an image file by name
	PngImageFile(const char* imageFileName);
    ///closes the image file
	virtual ~PngImageFile();
	
private:
    /* seems impossible to read PNG files one row at a time, starting at the
       bottom */
	TextureColor* image;

//- inherited from ImageFileBase
public:
	virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               Pixel* rectBuffer) const;
};

END_CRUSTA

#endif //_PngImageFile_H_
